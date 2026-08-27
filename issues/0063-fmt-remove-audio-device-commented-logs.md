# src/unity_audio_device.h のコメントアウトログと自明コメントを削除する

- Priority: Low
- Created: 2026-08-27
- Branch: fmt/remove-audio-device-commented-logs
- Polished: {YYYY-MM-DD}

## 目的

`src/unity_audio_device.h` に残っている `//RTC_LOG(...)` などのデバッグログのコメントアウトと、`//*audioLayer = ...` `//*available = true;` のような実装削除メモを掃除する。

## 現状

`src/unity_audio_device.h` の各所には次のような残骸が並んでいる。

- `AudioTransportImpl::RecordedDataIsAvailable` 直前の `//RTC_LOG(LS_INFO) << "AudioTransportImpl::RecordedDataIsAvailable:` 3 行
- `ActiveAudioLayer` 内の `//*audioLayer = AudioDeviceModule::kPlatformDefaultAudio;`
- 各種メソッドの前段に散在する `//*available = true;` などの旧実装記録
- 数十箇所の `//RTC_LOG(...)` コメントアウト

デバッグログとして残す意図があるなら本来はマクロ制御で残すべきだが、単純なコメントアウトのままで温存されており、broken windows に該当する。

## 設計方針

- `src/unity_audio_device.h` から `//RTC_LOG(...)` 系のコメントアウトを全て削除する
- `//*audioLayer = ...` などの旧実装メモを削除する
- 挙動変更は無い
- 別 issue で扱う AGENTS.md 規約対応（英語コメント日本語化）とは別のスコープで進める

## 完了条件

- `src/unity_audio_device.h` から `//RTC_LOG(...)` のコメントアウトが消えている
- 旧実装のコメントアウトメモが消えている
- Windows / macOS / iOS / Android / Ubuntu のビルドが通り、オーディオ機能に回帰が無い
