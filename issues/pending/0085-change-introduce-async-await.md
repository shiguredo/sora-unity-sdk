# Sora Unity SDK に async / await を導入する

- Created: 2026-09-11
- Completed: {YYYY-MM-DD}
- Branch: feature/change-introduce-async-await
- Polished: {YYYY-MM-DD}

## 目的

Sora Unity SDK の C# API のうち、完了や失敗を待つ必要があるものを Unity の awaitable (async / await) で扱えるようにする。現状は戻り値が `void` で、完了・失敗はコールバックと `Sora.DispatchEvents()` の定期呼び出しに依存しており、呼び出し側は待ち合わせを自前で書く必要がある。`await` で待てる API を提供し、アプリ側のコルーチンやコールバックに依存せずに非同期処理を記述できるようにする。

## 現状

- `SoraUnitySdkExamples/Assets/SoraUnitySdk/Sora.cs` の `Sora.Connect(Config)` は `void` を返し、接続処理の完了や成否を戻り値で受け取れない。接続後の通知は `Sora.DispatchEvents()` 経由で発火するコールバック (`OnDisconnect` 等) で扱う。
- `Sora.RequestRpc(string method, string paramsJson, Action<RpcResult> onResult, long timeoutMillis)` はコールバックで結果を受け取る。リクエスト ID の採番、レスポンスの受信、タイムアウト判定は `Sora.RequestRpc` と `Sora.HandleRpcInternal` が内部で行い、判定処理は `Sora.DispatchEvents()` から呼ばれる。呼び出し側は `Sora.DispatchEvents()` を継続的に呼び続ける必要がある。
- `Sora.GetStats(Action<string> onGetStats)` もコールバックで結果を受け取る。
- `Sora.SwitchCamera(CameraConfig)` や `Sora.Disconnect()` も `void` であり、完了を待つ手段がない。
- `SoraUnitySdkExamples/Assets/SoraSample.cs` では `StartCoroutine` / `IEnumerator` と `WaitForEndOfFrame` / `WaitForSeconds` で待ち合わせており、SDK の完了待ちとアプリ側のコルーチンが混在している。
- 既存のイベント処理は、ネイティブ側から `sora_dispatch_events` で Unity スレッドにイベントを積み、`Sora.DispatchEvents()` が Unity スレッド上で処理する設計になっている。

## 設計方針

- 対応する Unity バージョンは Unity 6000.3 (LTS) / 6000.0 (LTS) であり、Unity 2023.1 以降で導入された `UnityEngine.Awaitable` を利用できる。`System.Threading.Tasks.Task` ではなく、Unity が推奨する `Awaitable` を使う。
- 完了や失敗を待つ必要がある API (`Connect` / `Disconnect` / `SwitchCamera` / `RequestRpc` / `GetStats` など) を `await` で扱えるようにする。対象範囲は全 API を見直して決める。
- `await` の完了は Unity スレッド上で発生させる。既存の `Sora.DispatchEvents()` 連動のイベント処理を維持し、その流れの中で完了させる。
- 既存 API を `Awaitable` に変更するか、async 版の API を追加して既存 API を残すかを決める。破壊的変更の有無が変わるため、互換性の扱いを先に確定する。
- タイムアウトとキャンセルの扱いを決める。RPC は既に `timeoutMillis` を持つため、await 版でタイムアウトをどう表現するかを既存 API と揃える。
- 移行期間中の旧 API と新 API の併存方針 (同一バージョンで両方提供するか、複数バージョンで管理するか) を決める。

## 完了条件

- 接続・切断・RPC・Stats 取得など、完了待ちが必要な API を `await` で扱える
- `await` の完了が Unity スレッドで発生し、既存のイベントディスパッチ設計 (`Sora.DispatchEvents()`) と整合する
- 既存のコールバックベース API を利用しているアプリの挙動を変えない、または移行方針と提供期間が決まっている
- Windows / macOS / Android / iOS の各ターゲットのビルドが通る
- `CHANGES.md` の `## develop` に `[CHANGE]` エントリを追記する
- README または利用者向けドキュメントに async / await の利用方法を追記する

## pending にした理由

2026-09-11 に pending にする。

- 既存 API を `Awaitable` に変更するか、async 版を追加して既存 API を残すかで破壊的変更の有無が変わり、API の互換性をどう保つかの設計判断が必要である。
- `await` の完了を Unity スレッドで発生させる方法、タイムアウトとキャンセルの表現、旧 API と新 API の併存方針が未確定である。
- 対象 API の範囲が広く、`Connect()` のような既存 API の変更は利用者への影響が大きいため、これらの設計が決まるまで実装方針を確定できない。対応を再開するときは reopened にしてから、API の形と互換性の方針を確定する。
