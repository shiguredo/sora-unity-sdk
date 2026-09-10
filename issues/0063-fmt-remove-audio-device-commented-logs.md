# src/unity_audio_device.h のコメントアウトログと自明コメントを削除する

- Priority: Low
- Created: 2026-08-27
- Branch: fmt/remove-audio-device-commented-logs
- Polished: 2026-09-11

## 目的

`src/unity_audio_device.h` に残っている `//RTC_LOG(...)` などのデバッグログのコメントアウトと、`//*audioLayer = ...` `//*available = true;` のような実装削除メモを掃除する。

## 現状

`src/unity_audio_device.h` には残骸が次の 4 箇所ある。

- `AudioTransportImpl::RecordedDataIsAvailable` 先頭の `// RTC_LOG(LS_INFO)` から始まるデバッグログのコメントアウト 4 行
- `HandleAudioData` 内の `//RTC_LOG(LS_INFO) << "handle audio data: chunk_size=" ...` のコメントアウト 2 行
- `ActiveAudioLayer` 内の `//*audioLayer = AudioDeviceModule::kPlatformDefaultAudio;`
- `StereoPlayoutIsAvailable` 内の `//*available = true;`

これら以外にこのファイルには有効な `RTC_LOG(LS_INFO) << ...` 呼び出しが多数あるが、これらは動作に必要なログであり削除対象ではない。デバッグログとして残す意図があるなら本来はマクロ制御で残すべきだが、単純なコメントアウトのままで温存されており、broken windows に該当する。

## 設計方針

- `src/unity_audio_device.h` から `//RTC_LOG(` と `// RTC_LOG(` のコメントアウトを全て削除する
- `//*audioLayer = ...` などの旧実装メモを削除する
- 現時点で有効な `RTC_LOG` 呼び出しは残す
- 英語コメントの日本語化は別 issue で扱うため、本 issue では翻訳しない
- 挙動変更は無い

## 完了条件

- `src/unity_audio_device.h` から `//RTC_LOG(` と `// RTC_LOG(` のコメントアウトが消えている
- 旧実装メモ（`//*audioLayer` と `//*available`）が消えている
- 有効な `RTC_LOG` 呼び出しに変更が無い
- Windows / macOS / iOS / Android / Ubuntu のビルドが通り、オーディオ機能に回帰が無い
