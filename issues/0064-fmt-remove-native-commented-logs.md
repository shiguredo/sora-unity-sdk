# src/unity.cpp と src/unity_renderer.cpp のコメントアウトログを削除する

- Priority: Low
- Created: 2026-08-27
- Branch: fmt/remove-native-commented-logs
- Polished: 2026-09-11

## 目的

`src/unity.cpp` の `AudioTrackSinkImpl::OnData` に残っているコメントアウトログと、`src/unity_renderer.cpp` の `Sink::TextureUpdateCallback` 周辺および `Sink::~Sink` に散在する `//RTC_LOG(LS_INFO)` を削除する。これらはデバッグ用途のログを一時的にコメントアウトしたまま長期間放置されており、broken windows として扱う。

## 現状

`src/unity.cpp` の `AudioTrackSinkImpl::OnData`（`absolute_capture_timestamp_ms` 付きオーバーロード）の実装内に、次 4 行のコメントアウトが残っている。

```cpp
// RTC_LOG(LS_INFO) << "AudioTrackSinkImpl::OnData: bits_per_sample="
//                  << bits_per_sample << " sample_rate=" << sample_rate
//                  << " number_of_channels=" << number_of_channels
//                  << " number_of_frames=" << number_of_frames;
```

`src/unity_renderer.cpp` には次の 7 箇所 12 行のコメントアウトが残っている。

- `Sink::~Sink` の待機ループ内 1 行（`// RTC_LOG(LS_INFO) << ...` の 1 行）
- `Sink::TextureUpdateCallback` の Begin 分岐 4 箇所（`p == nullptr` 時、`deleting_` 時、`updating_ = true` 直後、`texData` 設定直後）
- `Sink::TextureUpdateCallback` の End 分岐 2 箇所（`p == nullptr` 時、`updating_ = false` 直前）

コメントアウトには `//RTC_LOG(` と `// RTC_LOG(` の 2 表記が混在している。

## 設計方針

- `src/unity.cpp` から `//RTC_LOG(` と `// RTC_LOG(` のコメントアウトを削除する
- `src/unity_renderer.cpp` から `//RTC_LOG(` と `// RTC_LOG(` のコメントアウトを削除する
- 必要なログは削除しない（現時点で有効なログはそのまま残す）
- 挙動変更は無い

## 完了条件

- 上記 2 ファイルから `//RTC_LOG(` と `// RTC_LOG(` のコメントアウトが消えている
- 有効なログには手を加えていない
- 全ターゲットでビルドが通る
