# SwitchCamera の future 待機によるデッドロックを解消する

- Priority: High
- Created: 2026-08-27
- Branch: fix/switch-camera-future-deadlock
- Polished: {YYYY-MM-DD}
- Milestone: 2026.2.0

## 目的

`Sora::SwitchCamera` の実装が Unity メインスレッドを無期限にブロックする経路を解消する。現状は `boost::asio::io_context` が停止した後に呼び出された場合、future が完了せずにハングする。

## 現状

`src/sora.cpp` の `Sora::SwitchCamera` は `boost::asio::post(*ioc_, ...)` で処理を投げ、その後 `f.wait()` で結果を待機する。Unity メインスレッドから呼ばれる前提で書かれているが、以下の経路でデッドロックが発生する。

- `ioc_->stop()` が既に呼ばれている（Disconnect 進行中や `~Sora` 直前）と `post` は実行されず、future が完了することはない
- `ioc_` を回している io_thread_ が終了済みの場合も同様
- `wait` にタイムアウトが無いため、Unity メインスレッドが永久にブロックされ、Unity アプリ全体がハングする

`SwitchCamera` は接続確立後のカメラ切り替え機能として広く使われる公開 API であり、Disconnect 前後のわずかな race window で踏まれる可能性がある。

## 設計方針

- `ioc_->stopped()` を最初に判定し、停止済みなら即座に失敗を返して return する
- `f.wait()` を `f.wait_for(std::chrono::seconds(...))` に置き換える
- タイムアウト到達時は失敗を返し、ハングを避ける
- io_thread_ が動作していない状態で呼ばれた場合の扱いをドキュメント化する

## 完了条件

- `Sora::SwitchCamera` が `ioc_->stopped()` を先に判定し、停止済みなら即座に return する
- future 待機がタイムアウト付きになっている
- Disconnect 進行中に `SwitchCamera` を呼んでも Unity メインスレッドが停止しない
- `CHANGES.md` の `## develop` に `[FIX]` を追記する
