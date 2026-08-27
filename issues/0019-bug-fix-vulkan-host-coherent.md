# Vulkan キャプチャに HOST_COHERENT を要求または vkInvalidateMappedMemoryRanges を追加する

- Priority: Critical
- Created: 2026-08-27
- Branch: fix/vulkan-host-coherent
- Polished: {YYYY-MM-DD}
- Milestone: 2026.2.0

## 目的

`UnityCameraCapturer::VulkanImpl` がメモリタイプ要求に `HOST_COHERENT` を含めておらず、`vkInvalidateMappedMemoryRanges` も呼んでいないため、Adreno / Mali などノンコヒーレントな HOST_VISIBLE メモリを引く端末では GPU の書き込みが CPU 側のキャッシュに反映されず、`vkMapMemory` 後の `std::memcpy` が古い / 未定義のバイト列を読む。Android Vulkan 環境で映像が化ける実バグ。Vulkan 1.3 Spec §10.2.1 違反。

## 現状

`src/unity_camera_capturer_vulkan.cpp` の `VulkanImpl::Init` は次のようにメモリタイプを要求する:

```cpp
int prop = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
for (uint32_t i = 0; i < mem_properties.memoryTypeCount; ++i) {
  int flags = mem_properties.memoryTypes[i].propertyFlags;
  if ((mem_requirements.memoryTypeBits & (1 << i)) &&
      (flags & prop) == prop) {
    allocInfo.memoryTypeIndex = i;
    found = true;
    break;
  }
}
```

要求しているのは `VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT` のみで、`VK_MEMORY_PROPERTY_HOST_COHERENT_BIT` を要求していない。

`VulkanImpl::Capture` は最後に `vkMapMemory` → `std::memcpy` でデータを読み取るが、その前に `vkInvalidateMappedMemoryRanges` を呼ぶ処理は存在しない。

Vulkan 1.3 Spec §10.2.1 "Host Access to Device Memory Objects" によると、HOST_COHERENT_BIT を含まないメモリタイプに対しては GPU 書き込み後に `vkInvalidateMappedMemoryRanges` を呼ばない限り、host が map したポインタから読み取った内容は未定義になる。

Qualcomm Adreno や ARM Mali などのモバイル GPU では HOST_VISIBLE かつ非 HOST_COHERENT のメモリタイプが提供されており、そちらが最初にマッチする端末では映像が化ける。

## 設計方針

- メモリタイプ要求に `VK_MEMORY_PROPERTY_HOST_COHERENT_BIT` を追加する

```cpp
int prop = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
```

- HOST_COHERENT を持つメモリタイプが見つからなかった場合のフォールバックとして、HOST_VISIBLE のみで allocate した上で `vkMapMemory` 直前に `vkInvalidateMappedMemoryRanges` を呼ぶ実装を用意する
  - HOST_COHERENT を含むメモリタイプが必ず存在する保証はないため、フォールバックを設けるのが安全
- Android の Adreno / Mali 端末で validation layer を有効化した Debug ビルドを走らせ、警告が出ないことを確認する

## 完了条件

- `VulkanImpl::Init` のメモリタイプ要求が `HOST_COHERENT` を含む
- または、非 HOST_COHERENT メモリを使う場合に `vkInvalidateMappedMemoryRanges` が map 前に呼ばれる
- Vulkan validation layer 有効化ビルドで警告なく動作する
- Adreno / Mali 系の Android 実機で映像が化けないことを確認する
- `CHANGES.md` の `## develop` に `[FIX] Vulkan キャプチャで HOST_COHERENT または vkInvalidateMappedMemoryRanges を扱うようにする` を追記する
