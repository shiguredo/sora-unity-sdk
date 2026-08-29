# Sora::ProcessAudio の unity_adm_ UAF を修正する

- Priority: Critical
- Created: 2026-08-27
- Branch: feature/fix-process-audio-unity-adm-uaf
- Polished: 2026-08-29
- Milestone: 2026.2.0

## 目的

`Sora.ProcessAudio` が Unity のオーディオスレッドなどのメインスレッド以外から呼ばれている最中に `Sora.Dispose()` が実行されると、`sora_process_audio` の `SoraWrapper*` 生ポインタ逆参照と、`~Sora` による `unity_adm_` の nullptr リセットが並行して走り、use-after-free が発生する。`Sora.Dispose()` を呼び出したタイミングと Unity のオーディオコールバックが競合するため、正式リリース前に必ず解消する。

## 現状

C# 側 `SoraUnitySdkExamples/Assets/SoraUnitySdk/Sora.cs` の `Sora.ProcessAudio` は、`src/unity.cpp` の C ABI 関数 `sora_process_audio` を呼ぶ。`sora_process_audio` は `SoraWrapper*` を生ポインタで逆参照して `Sora::ProcessAudio` を呼び出す:

```cpp
void sora_process_audio(void* p, const void* buf, int offset, int samples) {
  auto wsora = (SoraWrapper*)p;
  wsora->sora->ProcessAudio(buf, offset, samples);
}
```

`src/sora.cpp` の `Sora::ProcessAudio` は次のように `unity_adm_` を触る:

```cpp
void Sora::ProcessAudio(const void* p, int offset, int samples) {
  if (!unity_adm_) {
    return;
  }
  // 今のところステレオデータを渡すようにしてるので2倍する
  unity_adm_->ProcessAudioData((const float*)p + offset, samples * 2);
}
```

`unity_adm_` は `webrtc::scoped_refptr<UnityAudioDevice>` のメンバで、`~Sora` の中で以下のように nullptr 代入されて破棄される:

```cpp
capturer_sink_ = nullptr;
capturer_ = nullptr;
unity_adm_ = nullptr;
```

`Sora.Dispose()` は `sora_destroy` を呼び `SoraWrapper` を delete する。`SoraWrapper` が保持する `std::shared_ptr<Sora>` の最後の参照が落ちると `~Sora` が走り、上記の `unity_adm_ = nullptr` が実行される。

`Sora.ProcessAudio` はサンプル (SoraSample.cs の Render コルーチン) ではメインスレッドから呼ばれるが、`OnAudioFilterRead` のような Unity のオーディオコールバックから呼ぶとメインスレッドとは別のスレッド (オーディオスレッド) から実行される。`sora_process_audio` の `SoraWrapper*` 逆参照と `unity_adm_` の読み書きには、`Sora.Dispose()` 側の `delete SoraWrapper` / `~Sora` に対する同期が一切ない。

このため `Sora.Dispose()` の実行中に別スレッドの `Sora.ProcessAudio` が走ると、破棄済みの `SoraWrapper` または `Sora` を逆参照し、あるいは破棄済みの `UnityAudioDevice` を触って SEGV する。

## 設計方針

- `sora_process_audio` が `SoraWrapper*` を生ポインタで逆参照しているのが UAF の入口であるため、これを解消する
  - 0014 が `Sora::RenderCallbackStatic` に対して進めている方式 (IdPointer の weak_ptr 化と、呼び出し中 `std::shared_ptr<Sora>` を保持する設計) に揃える
  - IdPointer の weak_ptr 化は 0014 の範囲であり、本 issue は 0014 が解決済みであることを前提とする
  - `ProcessAudio` の実行中 `std::shared_ptr<Sora>` を保持し続ければ、`sora_destroy` と並行しても `~Sora` は走らず、`unity_adm_` を含むメンバへのアクセスは安全になる
  - C# 側 `Sora.ProcessAudio` も、新しい C ABI の形状 (ID ベースのルックアップ等) に追随させる
- `unity_adm_` は Connect 時に一度だけ設定され、Sora の生存中は不変で、`~Sora` でのみ nullptr にされる
  - 上記の寿命保証により `ProcessAudio` と `~Sora` の競合は原理的に消えるため、`unity_adm_` 自体の atomic 化は不要
  - 寿命保証と独立に `unity_adm_` を守る方針を採る場合のみ、C++20 の `std::atomic<std::shared_ptr<UnityAudioDevice>>` または `std::mutex` を検討する。ただし `UnityAudioDevice` は webrtc の `scoped_refptr` で管理されているため、shared_ptr 化する場合は webrtc の参照カウントとの整合が必要
- `Sora.cs` の `Dispose()` の順序変更 (adapter の破棄を先に行う等) は 0016 の範囲であり、本 issue では扱わない
- オーディオの破棄 race を扱う 0016 (AudioTransportImpl の audio_sink_ UAF) と 0017 (UnityAudioDevice の device_buffer_ race) とは対象が異なるが、Sora の寿命管理の方針は揃える

## 完了条件

- `sora_process_audio` が破棄済みの `SoraWrapper` / `Sora` を逆参照しないことを設計レベルで保証する
- `Sora::ProcessAudio` と `~Sora` の並行実行で UAF が発生しないことを設計レベルで保証する
- 別スレッドから `Sora.ProcessAudio` を呼び出している最中に `Sora.Dispose()` を実行しても SEGV しないことを確認する
  - この検証は 0017 (device_buffer_ race) が解決済みであることを前提とする。解決前は UnityAudioDevice 内部の race 経由で SEGV し得るため、単独で検証する場合は Sora の寿命と `unity_adm_` へのアクセスに範囲を限定する
- `Sora.Dispose()` と `Sora.ProcessAudio` を頻繁に繰り返すソークテストで SEGV せず、RSS が安定していることを確認する
- `CHANGES.md` の `## develop` に `[FIX] Sora::ProcessAudio の unity_adm_ UAF を修正する` を追記する
