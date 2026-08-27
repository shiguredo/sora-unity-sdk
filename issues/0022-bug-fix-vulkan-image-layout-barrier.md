# Vulkan image_ の layout 遷移バリアを実装する

- Priority: High
- Created: 2026-08-27
- Branch: fix/vulkan-image-layout-barrier
- Polished: {YYYY-MM-DD}
- Milestone: 2026.2.0

## 目的

`UnityCameraCapturer::VulkanImpl::Capture` で `image_` の layout 遷移バリアが実装されておらず、`VK_IMAGE_LAYOUT_UNDEFINED` のまま `vkCmdCopyImage` の dst として使用している (VUID-vkCmdCopyImage-dstImageLayout-00133 違反)。validation-strict なドライバでは失敗する経路のため、正式リリース前に barrier を実装する。

## 現状

`src/unity_camera_capturer_vulkan.cpp` の `VulkanImpl::Init` で `image_` は次のように作成される:

```cpp
imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
vkCreateImage(device, &imageCreateInfo, nullptr, &image_);
```

`VulkanImpl::Capture` は次のように `vkCmdCopyImage` の dst 引数に `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL` を渡している:

```cpp
vkCmdCopyImage(command_buffer,
               image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
               image_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
               1, &copyRegion);
```

しかし `image_` は作成後に `VK_IMAGE_LAYOUT_UNDEFINED` のままで、`TRANSFER_DST_OPTIMAL` に遷移させる `VkImageMemoryBarrier` は実装されていない。Vulkan Spec (VUID-vkCmdCopyImage-dstImageLayout-00133) では `dstImageLayout` は現在の実際のレイアウトと一致していなければならないと規定されており、UNDEFINED から直接 dst として使うのは不正。

ソース内には barrier の実装候補が 3 セット (`VulkanImpl::Capture` の各段階) コメントアウトされたまま残っており、実装として復活させる意図が窺える。

## 設計方針

- `VulkanImpl::Capture` に以下 3 セットの barrier を実装する
  - コピー前: `image_` を `UNDEFINED` -> `TRANSFER_DST_OPTIMAL` に遷移
  - コピー後 (map 前): `image_` を `TRANSFER_DST_OPTIMAL` -> `GENERAL` に遷移し host visible にする
  - もしくは初回のみ Init 中に `UNDEFINED` -> `TRANSFER_DST_OPTIMAL` に遷移させ、2 回目以降は同じレイアウトのまま繰り返し使う
- `VkImageMemoryBarrier` の `srcAccessMask` / `dstAccessMask` を適切に設定する
- コメントアウトされている 3 セットは、現在の実装フローに合わせて書き換える (単なる復活ではなく)
- `image_` の layout 状態を追跡するためのメンバ変数 (`current_layout_`) を追加することも検討する
- 別 issue で扱う HOST_COHERENT / queue 外部同期修正と併せて Vulkan 実装全体を再検証する

## 完了条件

- Vulkan validation layer 有効化ビルドで `VUID-vkCmdCopyImage-dstImageLayout-00133` 違反が発生しない
- Pixel / Samsung 系の validation-strict なドライバで動作することを確認する
- コメントアウトされた barrier ブロックが実装として復活しているか、あるいは明示的に削除されている
- `CHANGES.md` の `## develop` に `[FIX] Vulkan image_ の layout 遷移バリアを実装する` を追記する
