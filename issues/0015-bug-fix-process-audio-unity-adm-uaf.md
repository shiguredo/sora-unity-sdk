# Sora::ProcessAudio の unity_adm_ UAF を修正する

- Priority: Critical
- Created: 2026-08-27
- Branch: fix/process-audio-unity-adm-uaf
- Polished: {YYYY-MM-DD}
- Milestone: 2026.2.0

## 目的

`Sora::ProcessAudio` が Unity のオーディオスレッドから呼ばれる際、`~Sora` が並行して実行されると `unity_adm_` が nullptr にリセットされる過程で use-after-free が発生する。`Sora.Dispose()` を呼び出したタイミングと Unity のオーディオコールバックが競合するため、正式リリース前に必ず解消する。

## 現状

`src/sora.cpp` の `Sora::ProcessAudio` は次のように `unity_adm_` を触る:

```cpp
void Sora::ProcessAudio(const void* p, int offset, int samples) {
  if (unity_adm_ == nullptr) {
    return;
  }
  unity_adm_->ProcessAudioData((const float*)p + offset, samples * 2);
}
```

`unity_adm_` は `webrtc::scoped_refptr<UnityAudioDevice>` のメンバで、`~Sora` の中で以下のように nullptr 代入されて破棄される:

```cpp
capturer_sink_ = nullptr;
capturer_ = nullptr;
unity_adm_ = nullptr;
```

`ProcessAudio` は C ABI 経由で `sora_process_audio(void* p, ...)` から呼ばれ、Unity 側の `AudioRenderer` や `OnAudioFilterRead` などのオーディオコールバックで発火する。これは Unity のメインスレッドではなく専用のオーディオスレッドで実行されるが、`unity_adm_` の読み書きに対する同期は一切行われていない。

Unity 側で `Sora.Dispose()` を実行した直後、まだ実行中の別スレッド (オーディオスレッド) が `ProcessAudio` を叩き、`unity_adm_.get()` が返す `UnityAudioDevice*` を触ろうとすると、破棄済みメモリを参照して SEGV する。

## 設計方針

- `unity_adm_` を atomic に差し替え可能な形にする
  - `webrtc::scoped_refptr` のままでは atomic 操作ができないため、`std::atomic<std::shared_ptr<UnityAudioDevice>>` (C++20 の `atomic<shared_ptr>`) または独自のロックを組み合わせる
  - 選択肢としては (a) `std::shared_ptr` 化して atomic 操作、(b) `std::mutex` で `ProcessAudio` と `~Sora` を直列化、(c) `Sora` 自体を `weak_ptr` で守り、`ProcessAudio` が Sora の shared_ptr を lock する形にする
- Unity 側 C# `Sora.Dispose()` の順序も見直し、オーディオスレッドが停止していることを保証してから `sora_destroy` を呼ぶようにする
  - 別 issue で扱う AudioTransportImpl 生ポインタ問題と方針を揃える

## 完了条件

- `Sora::ProcessAudio` と `~Sora` の並行実行で UAF が発生しないことを設計レベルで保証する
- Unity のオーディオコールバックが動作している間に `Sora.Dispose()` を実行しても SEGV しないことを確認する
- `Sora.Dispose()` を頻繁に呼び出すソークテストで RSS が安定していることを確認する
- `CHANGES.md` の `## develop` に `[FIX] Sora::ProcessAudio の unity_adm_ UAF を修正する` を追記する
