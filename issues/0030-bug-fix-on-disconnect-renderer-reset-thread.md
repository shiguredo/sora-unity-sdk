# OnDisconnect の renderer_.reset() を Unity スレッドに寄せる

- Priority: High
- Created: 2026-08-27
- Branch: feature/fix-on-disconnect-renderer-reset-thread
- Polished: 2026-08-31
- Milestone: 2026.2.0

## 目的

`Sora::OnDisconnect` が IO スレッド上で `renderer_.reset()` を呼び出す実装を、Unity スレッドで実行するように寄せる。現状は Signaling 切断処理全体をブロックし、Unity スレッドで並行して呼ばれる `GetVideoTrackFromVideoSinkId` と race する。

## 現状

`src/sora.cpp` の `Sora::OnDisconnect` は SoraSignalingObserver のコールバックとして IO スレッドから呼ばれる（`SoraSignaling::SendOnDisconnect` が `ioc_` に post して呼び出す。`ioc_` は `Sora` の io_thread_ が `run()` する）。この中で `renderer_.reset()` を実行しているが、以下の問題がある。

- `UnityRenderer::Sink` のデストラクタは `deleting_ = true` セット後に `updating_` が false になるまで 10 ms 単位でスピン待ちする（`src/unity_renderer.cpp` の `~Sink`）
- IO スレッド上でこのスピンが発生すると、Signaling の切断処理全体が長時間ブロックされる
- Unity スレッドで並行して `GetVideoTrackFromVideoSinkId` などが呼ばれる場合、renderer_ が保持するコンテナへの操作と race する
- Sink の Native TextureUpdateCallback もこの間に発火しうる

## 設計方針

- `renderer_.reset()` を `PushEvent` に包み、DispatchEvents 経由で Unity スレッド上で実行する
- IO スレッドは PushEvent 後速やかに OnDisconnect を返し、Signaling 切断処理を先に進められるようにする
- 本 issue の対象は「OnDisconnect 起因の `renderer_.reset()` のスレッド移動」のみ。正常時の `DoSwitchCamera`（IO スレッド）の renderer_ アクセスは本 issue の対象外であり、「SwitchCamera の future 待機によるデッドロックを解消する」issue (0029) の late 実行ガードと合わせて扱う
- on_disconnect コールバックと reset の実行順を規定する
  - PushEvent に包んだラムダ内で `on_disconnect_` コールバックを**先に**実行し、その後に `renderer_.reset()` を実行する。ユーザーの on_disconnect ハンドラ実行中は renderer_ が生存し、ハンドラ内からの `GetVideoTrackFromVideoSinkId` 系の後始末が安全になる
- reset 後の Unity スレッドからのアクセスにも null ガードを入れる
  - `Sora::GetVideoTrackFromVideoSinkId` / `Sora::GetVideoSinkIdFromVideoTrack` は `renderer_` が null のまま operator-> すると SEGV する。両関数の先頭で `renderer_ == nullptr` を判定し、null なら `nullptr` / `0` を返す（C# 側の `Sora.GetVideoTrackFromVideoSinkId` は既存の `InvalidOperationException` 経路に乗る。C# サンプルの例外回避は「SoraSample の OnRemoveTrack で GetVideoTrackFromVideoSinkId の例外経路を回避する」issue (0034) の対象）
  - `Sora::OnTrack` / `Sora::OnRemoveTrack` の PushEvent ラムダ内でも `renderer_` に operator-> するため、`renderer_ == nullptr` なら早期 return するガードを入れる。ioc_ 停止後に残存トラックの `OnRemoveTrack` が発生して reset 後の dispatch になる場合でも SEGV させない
- Unity スレッドで ~Sink のスピン待ちが発生する点は、テクスチャ更新 1 回分（数十 ms オーダー）で bounded であり、IO スレッドをブロックしないことの方が重要であるため許容する

## 完了条件

- `Sora::OnDisconnect` で `renderer_.reset()` が Unity スレッド上（DispatchEvents 内）で実行されるようになっている
- IO スレッド上で Sink デストラクタのスピン待ちが発生しない
- on_disconnect ハンドラ実行中は renderer_ が生存し、ハンドラ内からの `GetVideoTrackFromVideoSinkId` 系呼び出しがクラッシュしない
- `renderer_` 破棄後（null）に Unity スレッドから `GetVideoTrackFromVideoSinkId` / `GetVideoSinkIdFromVideoTrack` が呼ばれても SEGV しない
- reset 後に `OnTrack` / `OnRemoveTrack` イベントが dispatch されても SEGV しない
- OnDisconnect 起因の renderer_ の破棄が IO スレッドと Unity スレッドの間で race しない（正常時の `DoSwitchCamera` の IO スレッドアクセスは対象外）
- `CHANGES.md` の `## develop` に `[FIX]` を追記する
