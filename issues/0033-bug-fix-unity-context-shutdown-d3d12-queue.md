# UnityContext の Shutdown で d3d12_command_queue_ を nullptr にクリアする

- Priority: High
- Created: 2026-08-27
- Branch: fix/unity-context-shutdown-d3d12-queue
- Polished: {YYYY-MM-DD}
- Milestone: 2026.2.0

## 目的

`src/unity_context.cpp` の Shutdown 処理で `d3d12_command_queue_` のクリアが漏れており、Shutdown 後に `GetD3D12CommandQueue()` が呼ばれると破棄済みの `ID3D12CommandQueue*` が返される。他のポインタと対称に nullptr クリアを追加する。

## 現状

`UnityContext::OnGraphicsDeviceEvent(kUnityGfxDeviceEventShutdown)` は Windows 向けに以下を実行している。

- `d3d11_device_context_->Release()` の後 `d3d11_device_context_ = nullptr;`
- `d3d11_device_ = nullptr;`
- `d3d12_device_ = nullptr;`

しかし `d3d12_command_queue_` のクリアは行われず、Unity から見て破棄済みのポインタが `UnityContext` のメンバに残り続ける。この状態で `GetD3D12CommandQueue()` が呼ばれると危険なポインタが返る。Unity が graphics device を切り替えたときも同様の問題が起きる。

## 設計方針

- `kUnityGfxDeviceEventShutdown` 分岐で `d3d12_command_queue_ = nullptr;` を追加する
- 対称的に Initialize 分岐と Shutdown 分岐でセット/クリアが揃うことを確認する
- 同時に Shutdown 中の `GetD3D12CommandQueue` 呼び出しに対する契約を README や docstring に明記する

## 完了条件

- `d3d12_command_queue_` が Shutdown で nullptr にクリアされる
- Shutdown 後の `GetD3D12CommandQueue()` が nullptr を返す
- Unity の graphics device 切り替え時にも同様に安全な状態になる
- `CHANGES.md` の `## develop` に `[FIX]` を追記する
