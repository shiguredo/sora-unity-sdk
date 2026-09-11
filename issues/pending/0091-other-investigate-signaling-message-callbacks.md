# OnSignalingMessage と OnWsClose を Unity SDK に追加するか検討する

- Created: 2026-09-11
- Completed: {YYYY-MM-DD}
- Branch: feature/debug-signaling-message-callbacks
- Polished: {YYYY-MM-DD}

## 目的

Sora C++ SDK には、主に Sora Python SDK の E2E テスト向けに OnSignalingMessage と OnWsClose が追加されている。Unity SDK でもこれらのコールバックを取得できるようにするかを検討する。

## 現状

- `src/sora.h` / `src/sora.cpp` の `Sora` クラスは `SoraSignalingObserver` のコールバックとして OnSetOffer / OnDisconnect / OnNotify / OnPush / OnMessage / OnRpc / OnTrack / OnRemoveTrack / OnDataChannel を実装している。
- C# 側 (`SoraUnitySdkExamples/Assets/SoraUnitySdk/Sora.cs`) も OnAddTrack / OnRemoveTrack / OnMediaStreamTrack / OnRemoveMediaStreamTrack / OnSetOffer / OnNotify / OnPush / OnMessage / OnDisconnect / OnDataChannel / OnHandleAudio / OnCapturerFrame を公開している。
- OnSignalingMessage (WebSocket で受信したシグナリングメッセージ全体) と OnWsClose (WebSocket の切断) は、C ABI (`src/unity.h` / `src/unity.cpp` の `sora_set_on_*`) にも C# 側にも公開されていない。
- Sora C++ SDK 2026.2.1 にはこれらのコールバックが存在する。

## 設計方針

- Unity SDK の利用者に OnSignalingMessage / OnWsClose が必要かを確認する。主な用途は E2E テストでのメッセージ検証と WebSocket 切断の検知と考えられる。
- 追加する場合は、既存コールバックと同じ形で次の 3 層に追加する。
  - `src/sora.h` / `src/sora.cpp` の `SetOnSignalingMessage` / `SetOnWsClose` と `SoraSignalingObserver` の実装
  - `src/unity.h` / `src/unity.cpp` の C ABI (`sora_set_on_signaling_message` / `sora_set_on_ws_close`)
  - `SoraUnitySdkExamples/Assets/SoraUnitySdk/Sora.cs` のコールバック公開と `sora_dispatch_events` 経由の呼び出し
- 追加しない場合は、その判断理由を残す。

## 完了条件

- OnSignalingMessage と OnWsClose を Unity SDK に追加するかどうかが決まっている。
- 追加する場合は、C# から両方のコールバックを取得できる。
- 追加しない場合は、不要と判断した理由が記録されている。

## pending にした理由

2026-09-11 に pending にする。

- 追加の要否が未判断であり、E2E テスト用途が中心のコールバックを Unity の公開 API に含めるかの設計判断が必要である。
- 公開 API を増やすと後方互換の維持対象になるため、利用者の需要を確認してから決める。
