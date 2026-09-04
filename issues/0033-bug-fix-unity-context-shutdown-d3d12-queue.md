# UnityContext の Shutdown で d3d12_command_queue_ を nullptr にクリアする

- Priority: High
- Created: 2026-08-27
- Branch: feature/fix-unity-context-shutdown-d3d12-queue
- Polished: 2026-09-04
- Milestone: 2026.2.0

## 目的

`src/unity_context.cpp` の Shutdown 処理で `d3d12_command_queue_` のクリアが漏れており、Shutdown 後に `GetD3D12CommandQueue()` が呼ばれると破棄済みの `ID3D12CommandQueue*` が返される。他のポインタと対称に nullptr クリアを追加する。

## 現状

`UnityContext::OnGraphicsDeviceEvent(kUnityGfxDeviceEventShutdown)` は Windows 向けに以下を実行している。

- `d3d11_device_context_->Release()` の後 `d3d11_device_context_ = nullptr;`
- `d3d11_device_ = nullptr;`
- `d3d12_device_ = nullptr;`

しかし `d3d12_command_queue_` のクリアは行われず、Unity から見て破棄済みのポインタが `UnityContext` のメンバに残り続ける。この状態で `GetD3D12CommandQueue()` が呼ばれると危険なポインタが返る。

- `d3d12_command_queue_` は Initialize 分岐で `IUnityGraphicsD3D12v4::GetCommandQueue()` の戻り値がセットされる。Unity 所有のポインタであり、`UnityContext` 側に Release 義務はない (クリアのみでよい)
- `GetD3D12CommandQueue()` の唯一の呼び出し元は `src/unity_camera_capturer_d3d12.cpp` の `D3D12Impl::Init` で、`device == nullptr || queue == nullptr` のチェックがあるため、nullptr が返れば capturer 作成が安全に失敗する
- `UnityContext::Shutdown` の呼び出し元は `src/unity.cpp` の `UnityPluginUnload` (プラグイン unload 時) のみ
- `UnityContext::Shutdown` と `GetD3D12CommandQueue` は同じ `mutex_` を取るため、Shutdown 実行中に他スレッドから `GetD3D12CommandQueue` が呼ばれた場合は Shutdown 完了までブロックされ、戻り値はクリア後の nullptr になる

## 設計方針

- `kUnityGfxDeviceEventShutdown` 分岐で `d3d12_command_queue_ = nullptr;` を追加する
- 対称的に Initialize 分岐と Shutdown 分岐でセット/クリアが揃うことを確認する
- 本 issue の対象は `UnityContext::d3d12_command_queue_` メンバのみ。既存の `D3D12Impl::queue_` (capturer 作成時に取得) が device 切り替え後に古いポインタを保持する問題は capturer のライフサイクルの問題であり本 issue の対象外
- `kUnityGfxDeviceEventBeforeReset` / `kUnityGfxDeviceEventAfterReset` 分岐は現行どおり空のまま (本 issue のスコープ外)
- `GetD3D12CommandQueue()` の契約を `src/unity_context.h` の宣言の doc コメントに明記する。README は API リファレンスを持たない構成のため記載先としない
  - 契約: Shutdown (`kUnityGfxDeviceEventShutdown`) 以降は nullptr を返す。呼び出し側は nullptr を扱えること

## 完了条件

- `d3d12_command_queue_` が Shutdown で nullptr にクリアされる
- Shutdown 後の `GetD3D12CommandQueue()` が nullptr を返す
- `src/unity_context.h` の `GetD3D12CommandQueue()` に「Shutdown 以降は nullptr を返す」契約が doc コメントとして明記されている
- Unity が graphics device 切り替え時に `kUnityGfxDeviceEventShutdown` を発行する場合も、同様に nullptr が返り安全である (Shutdown を発行しない経路は本 issue のスコープ外)
- `CHANGES.md` の `## develop` に `[FIX]` を追記する
