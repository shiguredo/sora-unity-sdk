# UnityAudioDevice の device_buffer_ / initialized_ の thread race を修正する

- Priority: Critical
- Created: 2026-08-27
- Branch: feature/fix-audio-device-buffer-race
- Polished: 2026-08-29
- Milestone: 2026.2.0

## 目的

`UnityAudioDevice::ProcessAudioData` が Unity スレッドから `device_buffer_->SetRecordedBuffer` を叩いている最中に、worker thread から `Terminate()` が並行して呼ばれると `device_buffer_.reset()` が完了して nullptr デリファレンスによる SEGV になる。`initialized_` / `is_recording_` は `std::atomic<bool>` だが、フラグ評価と `device_buffer_` の使用は別々の操作であり、評価を通過した直後に `Terminate()` が `device_buffer_.reset()` を実行すると後続の使用が破棄済みメモリを触る。フラグの atomicity だけでは防げないため、正式リリース前に必ず対応する。

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

`initialized_` / `is_recording_` / `is_playing_` は `std::atomic<bool>` だが、`device_buffer_` は `std::unique_ptr<webrtc::AudioDeviceBuffer>` であり、`ProcessAudioData` の「フラグ評価」と「`device_buffer_` の使用」は別々の操作として行われる。

Unity スレッドの `ProcessAudioData` が `initialized_ && is_recording_` を true と評価した直後、worker thread の `Terminate()` が `initialized_ = false` と `device_buffer_.reset()` を実行して完走すると、`ProcessAudioData` の後続で `device_buffer_->SetRecordedBuffer(...)` を呼ぶ瞬間には既に破棄済みメモリを触ることになる。フラグが atomic でも、評価と使用の間に明示的な同期がないためこの race は防げない。

## 設計方針

- `device_buffer_` を `std::atomic<std::shared_ptr<webrtc::AudioDeviceBuffer>>` に変更し、`ProcessAudioData` は `load()` でローカルにコピーしてから触る
  - プロジェクトは C++20 (`CMakeLists.txt` の `CXX_STANDARD 20`) のため利用可能
  - 単なる `std::shared_ptr` の並行 read / write は data race (UB) になるため、非 atomic の `std::shared_ptr` にはしない
  - `device_buffer_` の全使用箇所 (`HandleAudioData` の `RequestPlayoutData` / `GetPlayoutData`、`InitPlayout` / `InitRecording`、`RegisterAudioCallback` 等) も atomic アクセスに追随させる
- あるいは mutex を導入して `ProcessAudioData` と `Terminate` を直列化する
- `Terminate()` は `DoStopPlayout()` でプラウトスレッドを join してから `device_buffer_` を破棄する既存の順序を維持し、破棄そのものを参照カウントまたは直列化で保護する
  - 「フラグを先に false にする」だけでは既にフラグ評価を通過した `ProcessAudioData` を止められないため、フラグ操作では保護しない
- `initialized_` / `is_recording_` / `is_playing_` は既に `std::atomic<bool>` であり、変更しない
- 別 issue で扱う `Sora::ProcessAudio` の `unity_adm_` UAF と方針を揃える

## 完了条件

- `ProcessAudioData` と `Terminate` の並行実行で SEGV が発生しないことを設計レベルで保証する
- 録音中に `Sora.Dispose()` を呼び出すシナリオで nullptr デリファレンスが発生しないことを確認する
- `CHANGES.md` の `## develop` に `[FIX] UnityAudioDevice の device_buffer_ thread race を修正する` を追記する
