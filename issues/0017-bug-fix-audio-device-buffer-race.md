# UnityAudioDevice の device_buffer_ / initialized_ の thread race を修正する

- Priority: Critical
- Created: 2026-08-27
- Branch: fix/audio-device-buffer-race
- Polished: {YYYY-MM-DD}
- Milestone: 2026.2.0

## 目的

`UnityAudioDevice::ProcessAudioData` が Unity スレッドから `device_buffer_->SetRecordedBuffer` を叩いている最中に、worker thread から `Terminate()` が並行して呼ばれると `device_buffer_.reset()` が完了して nullptr デリファレンスによる SEGV になる。フラグに対する memory ordering の保証が欠けているため、正式リリース前に必ず対応する。

## 現状

`src/unity_audio_device.h` の `UnityAudioDevice::ProcessAudioData` は次のように動作する:

```cpp
void ProcessAudioData(const float* data, int32_t size) {
  if (!adm_recording_ && initialized_ && is_recording_) {
    for (int i = 0; i < size; i++) {
      converted_audio_data_.push_back(...);
    }
    int chunk_size = 48000 * 2 / 100;
    while (converted_audio_data_.size() > chunk_size) {
      device_buffer_->SetRecordedBuffer(converted_audio_data_.data(),
                                        chunk_size / 2);
      device_buffer_->DeliverRecordedData();
      ...
    }
  }
}
```

`Terminate()` は次のように動作する:

```cpp
virtual int32_t Terminate() override {
  DoStopPlayout();
  initialized_ = false;
  is_recording_ = false;
  is_playing_ = false;
  device_buffer_.reset();
  ...
}
```

`initialized_` / `is_recording_` / `is_playing_` はいずれも通常の `bool` (または非 atomic の類似型) で、`device_buffer_` は `std::unique_ptr<webrtc::AudioDeviceBuffer>` である。

Unity スレッドの `ProcessAudioData` が `initialized_ && is_recording_` を true と評価した直後、worker thread の `Terminate()` が `initialized_ = false` と `device_buffer_.reset()` を実行して完走すると、`ProcessAudioData` の後続で `device_buffer_->SetRecordedBuffer(...)` を呼ぶ瞬間には既に破棄済みメモリを触ることになる。

memory ordering の保証もなく、フラグ書き込みとバッファリセットの間に明示的な同期が入っていない。

## 設計方針

- `device_buffer_` を `std::atomic<std::shared_ptr<webrtc::AudioDeviceBuffer>>` (C++20) または独自の共有ロックで守り、atomic に差し替え可能な形にする
  - シンプルには `std::shared_ptr` にして、`ProcessAudioData` は shared_ptr をローカルにコピーしてから触る
- `initialized_` / `is_recording_` / `is_playing_` を `std::atomic<bool>` に変更する
- `Terminate()` は「フラグを先に false にする → 少し待って ProcessAudioData の in-flight を確認 → device_buffer_.reset()」のような順序で書く
  - あるいは mutex を導入して `ProcessAudioData` と `Terminate` を直列化する
- 別 issue で扱う `Sora::ProcessAudio` の `unity_adm_` UAF と方針を揃える

## 完了条件

- `ProcessAudioData` と `Terminate` の並行実行で SEGV が発生しないことを設計レベルで保証する
- 録音中に `Sora.Dispose()` を呼び出すテストで nullptr デリファレンスが発生しないことを確認する
- `CHANGES.md` の `## develop` に `[FIX] UnityAudioDevice の device_buffer_ thread race を修正する` を追記する
