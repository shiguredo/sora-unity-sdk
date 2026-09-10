# カメラキャプチャラの死んだコメントアウトブロックを削除する

- Priority: Low
- Created: 2026-08-27
- Branch: fmt/remove-camera-capturer-dead-comments
- Polished: 2026-09-11

## 目的

`src/unity_camera_capturer_vulkan.cpp` / `src/unity_camera_capturer_d3d11.cpp` / `src/unity_camera_capturer_d3d12.cpp` に残っている代替実装のコメントアウトブロックを削除し、broken windows を掃除する。

## 現状

### src/unity_camera_capturer_vulkan.cpp

`VulkanImpl::Capture` には `VkImageMemoryBarrier` を使ったレイアウト遷移のコメントアウトが 4 セット残っている (各セット 19 コメント行、計 76 コメント行)。内訳は次のとおり。

- `image.image` を `PRESENT_SRC_KHR` -> `TRANSFER_SRC_OPTIMAL` に遷移するセット (`vkCmdCopyImage` の前)
- `image_` を `UNDEFINED` -> `TRANSFER_DST_OPTIMAL` に遷移するセット (`vkCmdCopyImage` の前)
- `image_` を `TRANSFER_DST_OPTIMAL` -> `GENERAL` に遷移するセット (`vkCmdCopyImage` の後、map 前)
- `image.image` を `TRANSFER_SRC_OPTIMAL` -> `PRESENT_SRC_KHR` に遷移するセット (`vkCmdCopyImage` の後)

さらに `VulkanImpl::Capture` 末尾には、`AccessTexture` で `image_` を `VK_IMAGE_LAYOUT_GENERAL` で取り出し `image.memory.mapped` から直接読み取る旧実装案が 43 行コメントアウトされたまま残っている。現行実装は `vkGetImageSubresourceLayout` と `vkMapMemory` による読み取りであり、この旧実装案を参照する経路は無い。`VulkanImpl::Init` には行末の代替値注記 `// VK_IMAGE_TILING_OPTIMAL;` も残っている。

### src/unity_camera_capturer_d3d11.cpp

`D3D11Impl::Capture` には次のコメントアウトブロックが残っている。

- `//RTC_LOG(LS_INFO) << "GOT FRAME: pData=0x" ...` のデバッグログ 3 行
- `//libyuv::ARGBToI420((const uint8_t*)resource.pData, ...)` の別実装案 5 行 (`Map` した生データを直接変換する案。現行は上下反転後の `buf` を変換する)

### src/unity_camera_capturer_d3d12.cpp

`D3D12Impl::Capture` には `ResourceBarrier` を無効化するコメントアウトブロックが 2 箇所残っている (各 11 行)。

- `CopyTextureRegion` の前に、`// D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE → D3D12_RESOURCE_STATE_COPY_SOURCE` の遷移注記と `// D3D12_RESOURCE_BARRIER to_copy_barrier = {}` で始まる `ResourceBarrier` 呼び出しのコメントアウト
- `CopyTextureRegion` の後に、`// D3D12_RESOURCE_STATE_COPY_SOURCE → D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE` の遷移注記と `// D3D12_RESOURCE_BARRIER to_shader_barrier = {}` で始まる `ResourceBarrier` 呼び出しのコメントアウト

どちらも「全てのレンダリングが終わってからこの関数を呼び出してもらってるのでバリアは不要」という設計判断でコメントアウトされたものであり、復活させる計画は無い。

## 設計方針

- Vulkan のレイアウト遷移バリア 4 セットは、`image_` を対象とする 2 セットを実装として復活させ `image.image` を対象とする 2 セットを明示的に削除する作業が、Vulkan の layout バリアを扱う別 issue (VUID-vkCmdCopyImage-dstImageLayout-00133 対応) の完了条件に含まれている。そのため本 issue では barrier ブロックに触れない。別 issue の完了後に barrier のコメントアウトが残っていないことを確認する
- `VulkanImpl::Capture` 末尾の旧実装案ブロックと `VK_IMAGE_TILING_OPTIMAL` の注記は復活させる計画が無いため、本 issue で削除する
- D3D11 / D3D12 の代替実装コメントアウトは意図的に残す理由が無いので削除する
- D3D12 の「バリアは不要」という設計判断コメントは削除しない
- 挙動を変えない範囲でのコメント整理に留める

## 完了条件

- `src/unity_camera_capturer_vulkan.cpp` の `VulkanImpl::Capture` 末尾の旧実装案ブロックと、`VulkanImpl::Init` の `VK_IMAGE_TILING_OPTIMAL` 注記が消えている
- `src/unity_camera_capturer_d3d11.cpp` の `D3D11Impl::Capture` から `//RTC_LOG(...)` と `//libyuv::ARGBToI420(...)` のコメントアウトが消えている
- `src/unity_camera_capturer_d3d12.cpp` の `D3D12Impl::Capture` から `ResourceBarrier` のコメントアウトブロック 2 箇所が消えている (「バリアは不要」の設計判断コメントは残っている)
- Vulkan の layout バリアを扱う別 issue (VUID-vkCmdCopyImage-dstImageLayout-00133 対応) との役割分担が本 issue に明記されている (barrier コメントアウト 4 セットはその別 issue が復活または削除する)
- ビルドと基本的なキャプチャ動作が回帰していない
