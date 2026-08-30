# Vulkan image_ の layout 遷移バリアを実装する

- Priority: High
- Created: 2026-08-27
- Branch: feature/fix-vulkan-image-layout-barrier
- Polished: 2026-08-30
- Milestone: 2026.2.0

## 目的

`UnityCameraCapturer::VulkanImpl::Capture` で `image_` の layout 遷移バリアが実装されておらず、`VK_IMAGE_LAYOUT_UNDEFINED` のまま `vkCmdCopyImage` の dst として使用している (VUID-vkCmdCopyImage-dstImageLayout-00133 違反)。validation-strict なドライバでは失敗する経路のため、正式リリース前に barrier を実装する。

## 現状

`src/unity_camera_capturer_vulkan.cpp` の `VulkanImpl::Init` で `image_` は次のように作成される:

```cpp
imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
vkCreateImage(device, &imageInfo, nullptr, &image_);
```

`VulkanImpl::Capture` は次のように `vkCmdCopyImage` の dst 引数に `VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL` を渡している:

```cpp
vkCmdCopyImage(command_buffer,
               image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
               image_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
               1, &copyRegion);
```

しかし `image_` は作成後に `VK_IMAGE_LAYOUT_UNDEFINED` のままで、`TRANSFER_DST_OPTIMAL` に遷移させる `VkImageMemoryBarrier` は実装されていない。Vulkan Spec (VUID-vkCmdCopyImage-dstImageLayout-00133) では `dstImageLayout` は現在の実際のレイアウトと一致していなければならないと規定されており、UNDEFINED から直接 dst として使うのは不正。

ソース内には barrier の実装候補が 4 セット (`VulkanImpl::Capture` の各段階) コメントアウトされたまま残っており、実装として復活させる意図が窺える。うち 2 セットは `image_` を対象とし、残り 2 セットは Unity カメラテクスチャ `image.image` を対象としている。

## 設計方針

- `VulkanImpl::Capture` に以下 2 セットの barrier を実装する
  - コピー前: `image_` を `UNDEFINED` -> `TRANSFER_DST_OPTIMAL` に遷移
  - コピー後 (map 前): `image_` を `TRANSFER_DST_OPTIMAL` -> `GENERAL` に遷移してから host 読み取りを行う
- 上記 2 遷移は毎フレーム実行する。コピー前の `oldLayout` は 2 回目以降 `GENERAL` になるが、`UNDEFINED` を指定すれば内容は破棄され、画像全体は毎フレーム `vkCmdCopyImage` で上書きされるため問題ない
- `VkImageMemoryBarrier` の `srcAccessMask` / `dstAccessMask` を適切に設定する
- コメントアウトされた barrier ブロック 4 セットのうち、`image_` を対象とする 2 セット (`UNDEFINED` -> `TRANSFER_DST_OPTIMAL`、`TRANSFER_DST_OPTIMAL` -> `GENERAL`) を現在の実装フローに合わせて書き換えて実装する (単なる復活ではなく)
- 残る 2 セット (Unity カメラテクスチャ `image.image` の `PRESENT_SRC_KHR` -> `TRANSFER_SRC_OPTIMAL` と `TRANSFER_SRC_OPTIMAL` -> `PRESENT_SRC_KHR`) は、`AccessTexture` を `kUnityVulkanResourceAccess_PipelineBarrier` で呼んでいるため Unity 側が layout 遷移とバリアを処理しており、二重管理を避けるため復活させず明示的に削除する
- `image_` の layout 状態を追跡するメンバ変数 (`current_layout_`) は、毎フレーム `UNDEFINED` を起点とする上記方式では不要のため追加しない
- 別 issue で扱う HOST_COHERENT / queue 外部同期修正と併せて Vulkan 実装全体を再検証する

## 完了条件

- Vulkan validation layer 有効化ビルドで `VUID-vkCmdCopyImage-dstImageLayout-00133` 違反が発生しない
- Pixel / Samsung 系の validation-strict なドライバで動作することを確認する
- コメントアウトされた barrier ブロックのうち、`image_` を対象とする 2 セットが実装として復活し、`image.image` を対象とする 2 セットが明示的に削除されている
- `CHANGES.md` の `## develop` に `[FIX] Vulkan image_ の layout 遷移バリアを実装する` を追記する
