# Vulkan graphicsQueue の外部同期違反を解消する

- Priority: High
- Created: 2026-08-27
- Branch: fix/vulkan-queue-external-sync
- Polished: {YYYY-MM-DD}
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

さらに `vkQueueWaitIdle(queue)` は Unity の未完了描画も含めた全キューを stall させ、フレームレートを大きく落とす副作用がある。

Unity の推奨アプローチは `IUnityGraphicsVulkan::CommandRecordingState` / `IUnityGraphicsVulkan::ConfigureEvent` 系の API を使うことで、直接 submit する現在の実装は Unity ドキュメント上も非推奨と明記されている。

## 設計方針

- `IUnityGraphicsVulkan::ConfigureEvent` を使い、Unity の描画イベントに sync させたコールバック内で command buffer を組み立てる形に変更する
  - Unity 側が queue submission を制御するため、外部同期違反が解消される
- あるいは、独自の `VkQueue` を専用に作成する
  - `vkGetDeviceQueue` は同じ index に対して同じ queue を返すため、専用 queue を作るには queueCreateInfo の段階で queue index を分ける必要があり、Unity が制御している VkDevice に対してはできない
  - このため実質的には `ConfigureEvent` 方式が現実解
- `vkQueueWaitIdle` を CPU fence 相当の `VkFence` + `vkWaitForFences` に置き換え、他の queue submission に対する影響を最小化する

## 完了条件

- Vulkan validation layer 有効化ビルドで queue の外部同期違反警告が出ない
- Android Vulkan 環境で Unity 描画のフレームレート低下が緩和されている
- Vulkan キャプチャの動作が変わっていない (正しい映像がキャプチャされる)
- `CHANGES.md` の `## develop` に `[FIX] Vulkan graphicsQueue の外部同期違反を解消する` を追記する
