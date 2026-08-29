# Vulkan キャプチャに HOST_COHERENT を要求または vkInvalidateMappedMemoryRanges を追加する

- Priority: Critical
- Created: 2026-08-27
- Branch: feature/fix-vulkan-host-coherent
- Polished: 2026-08-29
- Milestone: 2026.2.0

## 目的

`UnityCameraCapturer::VulkanImpl` がメモリタイプ要求に `HOST_COHERENT` を含めておらず、`vkInvalidateMappedMemoryRanges` も呼んでいないため、Adreno / Mali などノンコヒーレントな HOST_VISIBLE メモリを引く端末では GPU の書き込みが CPU 側のキャッシュに反映されず、`vkMapMemory` 後の `std::memcpy` が古い / 未定義のバイト列を読む。Android Vulkan 環境で映像が化ける実バグ。Vulkan 1.3 Spec §11.2.16 違反。

## 現状

`src/unity_camera_capturer_vulkan.cpp` の `VulkanImpl::Init` は次のようにメモリタイプを要求する:

```cpp
int prop = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
for (uint32_t i = 0; i < mem_properties.memoryTypeCount; ++i) {
  int flags = mem_properties.memoryTypes[i].propertyFlags;
  RTC_LOG(LS_INFO) << "type[" << i << "]=" << flags;
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

Vulkan 1.3 Spec §11.2.16 "Host Access to Device Memory Objects" によると、HOST_COHERENT_BIT を含まないメモリタイプに対しては GPU 書き込み後に `vkInvalidateMappedMemoryRanges` を呼ばない限り、host が map したポインタから読み取った内容は未定義になる。

Qualcomm Adreno や ARM Mali などのモバイル GPU では HOST_VISIBLE かつ非 HOST_COHERENT のメモリタイプが最初にマッチする場合があり、その場合は映像が化ける。

## 設計方針

- メモリタイプ要求に `VK_MEMORY_PROPERTY_HOST_COHERENT_BIT` を追加する

```cpp
int prop = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
```

- HOST_COHERENT を持つメモリタイプが見つからなかった場合のフォールバックとして、HOST_VISIBLE のみで allocate した上で、GPU の書き込み完了 (`vkQueueWaitIdle`) を待った後、`vkMapMemory` と `std::memcpy` による読み取りの前に `vkInvalidateMappedMemoryRanges` を呼ぶ実装を用意する
  - Vulkan 1.3 Spec は HOST_VISIBLE + HOST_COHERENT のメモリタイプが必ず 1 つ存在することを要求しているが、それが `image_` の `memoryTypeBits` に含まれる保証はないため、見つからなかった場合のフォールバックを設ける
- queue 外部同期 (別 issue) と `image_` の layout バリア (別 issue) は同じ `VulkanImpl::Init` / `VulkanImpl::Capture` を変更対象とするため、実装順序と相互影響を調整する
- Android の Adreno / Mali 端末で validation layer を有効化した Debug ビルドを走らせ、警告が出ないことを確認する

## 完了条件

- `VulkanImpl::Init` のメモリタイプ要求が `HOST_COHERENT` を含む
- または、非 HOST_COHERENT メモリを使う場合に、GPU の書き込み完了を待った後、読み取り前に `vkInvalidateMappedMemoryRanges` が呼ばれる
- Vulkan validation layer 有効化ビルドで、この修正によって新たな警告や VUID 違反が発生しないこと (本バグは validation layer では検出できないため、修正の成否は次の実機確認で判定する)
- Adreno / Mali 系の Android 実機で映像が化けないことを確認する
- `CHANGES.md` の `## develop` に `[FIX] Vulkan キャプチャで HOST_COHERENT または vkInvalidateMappedMemoryRanges を扱うようにする` を追記する
