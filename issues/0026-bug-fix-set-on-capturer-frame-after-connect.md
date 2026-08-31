# SetOnCapturerFrame が Connect 後の null 化で NRE を起こす問題を修正する

- Priority: High
- Created: 2026-08-27
- Branch: fix/set-on-capturer-frame-after-connect
- Polished: 2026-08-31
- Milestone: 2026.2.0

## 目的

`Sora::DoConnect` は `on_capturer_frame_` の std::function を on_frame ラムダに**値キャプチャ**しているため、Connect 後に C# 側で `SetOnCapturerFrame(null)` を呼んでもキャプチャラ内部の callback は解除されず、Managed 側の `onCapturerFrame` が null になった状態で native からコールバックが飛ぶと `sora!.onCapturerFrame!(frame)` で NullReferenceException になる。

## 現状

`src/sora.cpp` の `Sora::DoConnect` は次のように on_frame ラムダを構築し `CreateVideoCapturer` に渡している (概要):

```cpp
auto on_frame = [on_capturer_frame = on_capturer_frame_](const webrtc::VideoFrame& frame) {
  if (on_capturer_frame) {
    on_capturer_frame(...);
  }
};
CreateVideoCapturer(..., on_frame, ...);
```

`on_capturer_frame_` は値キャプチャされ、以降キャプチャラ内部はこのコピーを保持し続ける。`SetOnCapturerFrame(nullptr)` を Connect 後に呼び出しても、キャプチャラ内部の `on_capturer_frame` は元の関数ポインタを叩き続ける。

C# 側 `SoraUnitySdkExamples/Assets/SoraUnitySdk/Sora.cs` の `OnCapturerFrame` setter は次のような順序で処理する:

```csharp
set {
    onCapturerFrame = value;                                    // 先に C# 側フィールドを null にする
    sora_set_on_capturer_frame(p, value == null ? null : CapturerFrameCallback, GCHandle.ToIntPtr(selfHandle));
}
```

ネイティブが unregister を無視するのに対し、C# 側は既に `onCapturerFrame = null;` になっている。次の native callback が来ると `CapturerFrameCallback` が `sora!.onCapturerFrame!(frame)` を実行し、force-unwrap した null 参照で NullReferenceException が発火する。

同じパターンで `on_handle_audio_` も値渡しされ、C# 側 `HandleAudioCallback` も `sora!.onHandleAudio!(...)` の force-unwrap を使うため、Connect 後に `SetOnHandleAudio(nullptr)` を呼ぶと `on_capturer_frame_` と同等の NRE 経路が存在する。ただし本 issue の対象は `on_capturer_frame_` のみとし、`on_handle_audio_` の修正は別 issue で対応する。

また `Sora::SwitchCamera` も同じパターンで on_frame ラムダを構築している。SwitchCamera は接続中にカメラを切り替える API であり、新キャプチャラには「その時点の」`on_capturer_frame_` が再び値キャプチャされるため、SwitchCamera 後に呼んだ `SetOnCapturerFrame` の変更も反映されない。

## 設計方針

- `Sora::DoConnect` と `Sora::SwitchCamera` の on_frame ラムダで `on_capturer_frame_` を値キャプチャせず、`[weak_this = weak_from_this()]` 経由の動的読取に変更する

```cpp
auto on_frame = [weak_this = weak_from_this()](const webrtc::VideoFrame& frame) {
  if (auto self = weak_this.lock()) {
    // ロックを取ってコールバックをローカル変数へコピーし、ロック解放後に呼び出す
    std::function<void(std::string)> on_capturer_frame;
    {
      std::lock_guard<std::mutex> guard(self->callback_mutex_);
      on_capturer_frame = self->on_capturer_frame_;
    }
    if (on_capturer_frame) {
      on_capturer_frame(...);
    }
  }
};
```

- これにより `SetOnCapturerFrame(nullptr)` を Connect 後に呼ぶと、次回 on_frame ではローカルコピーが空になり呼び出しがスキップされる
- on_frame ラムダは現在値に関わらず常にインストールする。現状の `if (on_capturer_frame_) { on_frame = ... }` という構築時ガードは除去する (Connect 後に初めて `SetOnCapturerFrame` でコールバックを設定するケースでも動的に反映されるようにするため)
- 動的読取化に伴い `on_capturer_frame_` の読み書きが並行する。書き込み側は `SetOnCapturerFrame` (Unity スレッド)、読み取り側は on_frame (signaling スレッド) であり、`std::function` の並行 read / write は data race (UB) になる。`sora.h` に `std::mutex callback_mutex_` を追加し、`SetOnCapturerFrame` と on_frame の両側でロックする。on_frame 内ではロックを取って `on_capturer_frame_` をローカル変数へコピーしてからロックを解放し、コピーを呼び出す。ロック保持中のユーザーコールバック呼び出しは deadlock の恐れがあるため行わない
- 同時に C# 側 `CapturerFrameCallback` の force-unwrap `sora!.onCapturerFrame!(frame)` に null チェックを追加する (`sora?.onCapturerFrame?.Invoke(frame)`)
- `on_handle_audio_` の修正は本 issue の対象外とし、別 issue で対応する

## 完了条件

- Connect 後に `SetOnCapturerFrame(null)` を呼び出しても NRE が発生しない (SwitchCamera でキャプチャラを再作成した場合も含む)
- `SetOnCapturerFrame` で設定した `on_capturer_frame_` の値が、キャプチャラ経由のコールバックに動的に反映される
- `SetOnCapturerFrame` (Unity スレッド) と on_frame 呼び出し (signaling スレッド) が並行しても data race (未定義動作) が発生しない
- C# 側 `CapturerFrameCallback` の null チェックが強化されている
- `CHANGES.md` の `## develop` に `[FIX] SetOnCapturerFrame が Connect 後の null 化で NRE を起こす問題を修正する` を追記する
