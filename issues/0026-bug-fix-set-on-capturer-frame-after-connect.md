# SetOnCapturerFrame が Connect 後の null 化で NRE を起こす問題を修正する

- Priority: High
- Created: 2026-08-27
- Branch: fix/set-on-capturer-frame-after-connect
- Polished: {YYYY-MM-DD}
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

同じパターンで `on_handle_audio_` も値キャプチャしているが、こちらは Connect 時のクロージャで確定するため、実質的に値の変更が反映されない設計になっている。`on_capturer_frame_` だけは実際に NRE 経路が存在する。

## 設計方針

- `Sora::DoConnect` の on_frame ラムダで `on_capturer_frame_` を値キャプチャせず、`[weak_this = weak_from_this()]` 経由の動的読取に変更する

```cpp
auto on_frame = [weak_this = weak_from_this()](const webrtc::VideoFrame& frame) {
  if (auto self = weak_this.lock()) {
    if (self->on_capturer_frame_) {
      self->on_capturer_frame_(...);
    }
  }
};
```

- これにより `SetOnCapturerFrame(nullptr)` を Connect 後に呼ぶと、次回 on_frame では `on_capturer_frame_` が false になり呼び出しがスキップされる
- 同時に C# 側 `CapturerFrameCallback` の force-unwrap `sora!.onCapturerFrame!(frame)` に null チェックを追加する (`sora?.onCapturerFrame?.Invoke(frame)`)
- `on_handle_audio_` についても同じパターンで見直すか、少なくとも「Connect 中は変更できない」ことを doc で明示する

## 完了条件

- Connect 後に `SetOnCapturerFrame(null)` を呼び出しても NRE が発生しない
- `on_capturer_frame_` の値が動的に反映される
- C# 側 `CapturerFrameCallback` の null チェックが強化されている
- `CHANGES.md` の `## develop` に `[FIX] SetOnCapturerFrame が Connect 後の null 化で NRE を起こす問題を修正する` を追記する
