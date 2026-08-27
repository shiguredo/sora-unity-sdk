# OnDisconnect の renderer_.reset() を Unity スレッドに寄せる

- Priority: High
- Created: 2026-08-27
- Branch: fix/on-disconnect-renderer-reset-thread
- Polished: {YYYY-MM-DD}
- Milestone: 2026.2.0

## 目的

`Sora::OnDisconnect` が IO スレッド上で `renderer_.reset()` を呼び出す実装を、Unity スレッドで実行するように寄せる。現状は Signaling 切断処理全体をブロックし、Unity スレッドで並行して呼ばれる `GetVideoTrackFromVideoSinkId` と race する。

## 現状

`src/sora.cpp` の `Sora::OnDisconnect` は SoraSignalingObserver のコールバックとして IO スレッドから呼ばれる。この中で `renderer_.reset()` を実行しているが、以下の問題がある。

- `UnityRenderer::Sink` のデストラクタは `deleting_ = true` セット後に `updating_` が false になるまで 10 ms 単位でスピン待ちする（`src/unity_renderer.cpp` の `~Sink`）
- IO スレッド上でこのスピンが発生すると、Signaling の切断処理全体が長時間ブロックされる
- Unity スレッドで並行して `GetVideoTrackFromVideoSinkId` などが呼ばれる場合、renderer_ が保持するコンテナへの操作と race する
- Sink の Native TextureUpdateCallback もこの間に発火しうる

## 設計方針

- `renderer_.reset()` を `PushEvent` に包み、DispatchEvents 経由で Unity スレッド上で実行する
- IO スレッドは PushEvent 後速やかに OnDisconnect を返し、Signaling 切断処理を先に進められるようにする
- Unity スレッド側で renderer_ が破棄されるまでの間の一時的な参照アクセスが安全であることを確認する

## 完了条件

- `Sora::OnDisconnect` で `renderer_.reset()` が Unity スレッド上で実行されるようになっている
- IO スレッド上で Sink デストラクタのスピン待ちが発生しない
- Unity スレッドと IO スレッドの間で renderer_ の race が発生しない
- `CHANGES.md` の `## develop` に `[FIX]` を追記する
