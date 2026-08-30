# Vulkan graphicsQueue の外部同期違反を解消する

- Priority: High
- Created: 2026-08-27
- Branch: feature/fix-vulkan-queue-external-sync
- Polished: 2026-08-30
- Milestone: 2026.2.0

## 目的

`UnityCameraCapturer::VulkanImpl` が Unity 共有の `graphicsQueue` に対してロックなしで `vkQueueSubmit` / `vkQueueWaitIdle` を呼び出しており、Vulkan 1.3 Spec §3.6 の外部同期契約 (VkQueue is Externally Synchronized) に違反している。また `vkQueueWaitIdle` は Unity 描画全体を stall させる副作用があり、フレームレートに深刻な影響を与える。

## 現状

`src/unity_camera_capturer_vulkan.cpp` の `VulkanImpl::Capture` は次のように動作する:

```cpp
IUnityGraphicsVulkan* graphics =
    context_->GetInterfaces()->Get<IUnityGraphicsVulkan>();
UnityVulkanInstance instance = graphics->Instance();
VkDevice device = instance.device;
VkQueue queue = instance.graphicsQueue;
...
vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
vkQueueWaitIdle(queue);
```

`IUnityGraphicsVulkan::Instance().graphicsQueue` は Unity 自身も描画コマンドを流している共有の `VkQueue`。Vulkan 1.3 Spec §3.6 "Threading Behavior" の Externally Synchronized Parameters 表で `VkQueue` は外部同期対象と定義されており、同一 queue に対する `vkQueueSubmit` / `vkQueueSubmit2` / `vkQueueWaitIdle` / `vkQueuePresentKHR` は複数スレッドから同時に呼び出せない。Unity 内部の submission と衝突する可能性がある。

さらに `vkQueueWaitIdle(queue)` は、その graphicsQueue 上で Unity が submit した未完了の描画コマンドも含めて queue 全体の処理完了までホスト側をブロックするため、フレームレートを大きく落とす副作用がある。

Unity のネイティブプラグイン API (`src/unity/IUnityGraphicsVulkan.h`) では、plugin event callback から `graphicsQueue` への submission は `ConfigureEvent` で `kUnityVulkanGraphicsQueueAccess_Allow` に設定したイベントか `AccessQueue` 経由のみが許可されており、デフォルト状態のイベントコールバックからは "no work must be submitted to UnityVulkanInstance::graphicsQueue from the plugin event callback" と submit が禁止されている。また `CommandRecordingState` は Unity の現在の command buffer へ直接 recording して Unity 側に submit させる API であり、`ConfigureEvent` / `CommandRecordingState` / `AccessQueue` 系の API を使ったフローが Unity の推奨アプローチとなる。現在の実装はこれらを使わずデフォルト状態のイベントコールバックから直接 `vkQueueSubmit` しており、Unity の定める契約に反している。

## 設計方針

- `IUnityGraphicsVulkan::ConfigureEvent` で描画イベントを `kUnityVulkanGraphicsQueueAccess_Allow` に設定し、そのイベントコールバック内でのみ自前の command buffer の組み立てと `vkQueueSubmit` を行う形に変更する
  - この設定のイベントコールバック実行時は Unity が graphicsQueue への排他アクセスを保証するため、外部同期違反が解消される
  - `AccessTexture` は queue access が有効なイベントコールバック内からは呼べない (`src/unity/IUnityGraphicsVulkan.h` の `AccessTexture` に "Must not be called from event callbacks configured for queue access" と明記) ため、`AccessTexture` によるテクスチャ取得・バリア挿入を行う文脈と、command buffer の submit を行う文脈を分離して実装する
- あるいは、独自の `VkQueue` を専用に作成する
  - `vkGetDeviceQueue` は同じ index に対して同じ queue を返すため、専用 queue を作るには queueCreateInfo の段階で queue index を分ける必要があり、Unity が制御している VkDevice に対してはできない
  - このため実質的には `ConfigureEvent` 方式が現実解
- `vkQueueWaitIdle` をやめ、自前の submit に `VkFence` を渡し `vkWaitForFences` で完了を待つように変更する
  - 待機が queue 全体ではなく自分の submission の完了だけに限定されるため、Unity の描画への影響を最小化できる
- HOST_COHERENT 修正 (別 issue) と `image_` の layout バリア (別 issue) は同じ `VulkanImpl::Init` / `VulkanImpl::Capture` を変更対象とするため、実装順序と相互影響を調整する

## 完了条件

- Vulkan validation layer 有効化ビルドで queue の外部同期違反警告が出ない
- Android Vulkan 環境で Unity 描画のフレームレート低下が緩和されている
- Vulkan キャプチャの動作が変わっていない (正しい映像がキャプチャされる)
- `CHANGES.md` の `## develop` に `[FIX] Vulkan graphicsQueue の外部同期違反を解消する` を追記する
