# SwitchCamera の future 待機によるデッドロックを解消する

- Priority: High
- Created: 2026-08-27
- Branch: fix/switch-camera-future-deadlock
- Polished: 2026-08-31
- Milestone: 2026.2.0

## 目的

`Sora::SwitchCamera` の実装が Unity メインスレッドを無期限にブロックする経路を解消する。現状は `boost::asio::io_context` が停止した後に呼び出された場合、future が完了せずにハングする。切断後も `set_offer_` は true のままのため、切断後の通常呼び出しでも決定的にハングする。

## 現状

`src/sora.cpp` の `Sora::SwitchCamera` は次の流れで動作する。

1. `if (!set_offer_) return;`
2. 旧 capturer の `Stop()` を同期実行
3. `CreateVideoCapturer` で新 capturer を作成
4. `boost::asio::post(*ioc_, ...)` で `DoSwitchCamera` を投げる
5. `f.wait()` で IO スレッドの完了を無期限に待機

post したラムダは `&p` を参照キャプチャし、`DoSwitchCamera` 実行後に `p.set_value()` を呼ぶ。`f.wait()` にタイムアウトはない。

以下の経路で `f.wait()` が完了せず、Unity メインスレッド（C# `Sora.cs` の `SwitchCamera` からの同期呼び出し）が永久にブロックされる。

- `ioc_->stop()` は `Sora::OnDisconnect` と `~Sora` でのみ呼ばれる。停止済みの io_context に post されたハンドラは実行されず、future は完了しない
- `set_offer_` は `Sora::OnSetOffer` で true になるのみで、`Sora::OnDisconnect` では false に戻らない。このため切断後も `SwitchCamera` は 1 のガードを通過し、`f.wait()` で決定的にハングする

つまり「わずかな race window」ではなく、切断後の通常呼び出しでも再現し得る。

## 設計方針

- `ioc_->stopped()` を追加し、停止済みなら `DoSwitchCamera` への post を行わず return する
  - 0028 の不変条件（全復帰経路で return 前に旧 capturer の `Stop()` を完了する）を満たすため、`stopped()` 判定は**旧 capturer の Stop() を完了してから**行う。すなわち現状の 2 の Stop 実行後、3 の `CreateVideoCapturer` より前に判定を置く
- `f.wait()` を `f.wait_for(std::chrono::seconds(5))` に置き換える
  - `DoSwitchCamera` の作業（AddTrack / ReplaceTrack / SetTrack）は短時間で完了するため 5 秒で十分であり、Unity メインスレッドのブロックも実用上許容できる
  - `ioc_->stopped()` は瞬間判定のため、判定と post の間に並行して `stop()` が呼ばれる競合はタイムアウトが最終防衛線になる
- タイムアウト時の safe return を保証する
  - post 済みラムダは `&p` 参照キャプチャのため、タイムアウトで return した後も IO スレッドがラムダを実行すると**破棄済み promise への `set_value()`** になり未定義動作を起こす。また `DoSwitchCamera` の late 実行は `renderer_` / `video_sender_` / `video_track_` を破壊する
  - promise を共有状態（`std::shared_ptr`）で保持し、タイムアウトで放棄された場合はラムダが `DoSwitchCamera` を実行しないガードを入れる。放棄後の `set_value` や late 実行による状態破壊を防ぐ
- 現 API は `void` で失敗を戻せないため、タイムアウトや停止時は `RTC_LOG(LS_ERROR)` にログを残して return する（呼び出し元への失敗伝達は行わない）。`Sora.cs` 側の `SwitchCamera` の doc コメントに、切断後は呼び出しても失敗するだけである旨を追記する

## 完了条件

- `Sora::SwitchCamera` が `ioc_->stopped()` を早期に判定し、停止済みの場合も旧 capturer の `Stop()` を完了してから return する
- future 待機が 5 秒のタイムアウト付きになる
- タイムアウト後に post 済みラムダが実行されても、破棄済み promise への `set_value` や `DoSwitchCamera` の late 実行による破壊的アクセスが起きない
- Disconnect 進行中または切断後に `SwitchCamera` を呼んでも Unity メインスレッドが永久ブロックされない（最大 5 秒で復帰する）
- `CHANGES.md` の `## develop` に `[FIX]` を追記する
