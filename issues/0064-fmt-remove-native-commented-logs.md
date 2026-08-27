# src/unity.cpp と src/unity_renderer.cpp のコメントアウトログを削除する

- Priority: Low
- Created: 2026-08-27
- Branch: fmt/remove-native-commented-logs
- Polished: {YYYY-MM-DD}

## 目的

`src/unity.cpp` の `AudioTrackSinkImpl::OnData` に残っているコメントアウトログと、`src/unity_renderer.cpp` の `Sink::TextureUpdateCallback` 周辺に散在する `//RTC_LOG(LS_INFO)` を削除する。

## 現状

`src/unity.cpp` の `AudioTrackSinkImpl::OnData` の実装内に、旧デバッグログとして `//RTC_LOG(LS_INFO) << "AudioTrackSinkImpl::OnData: ...` のコメントアウトが残っている。

`src/unity_renderer.cpp` の `Sink::TextureUpdateCallback` の Begin 分岐と End 分岐、および `Sink::~Sink` の待機ループ内など、複数箇所に `//RTC_LOG(LS_INFO)` のコメントアウトが残っている。

これらはデバッグ用途のログを一時的にコメントアウトしたまま長期間放置されている状態で、broken windows として扱う。

## 設計方針

- `src/unity.cpp` の `//RTC_LOG(...)` コメントアウトを削除する
- `src/unity_renderer.cpp` の `//RTC_LOG(...)` コメントアウトを削除する
- 必要なログは削除しない（現時点で有効なログはそのまま残す）
- 挙動変更は無い

## 完了条件

- 上記 2 ファイルから `//RTC_LOG(...)` のコメントアウトが消えている
- 有効なログには手を加えていない
- 全ターゲットでビルドが通る
