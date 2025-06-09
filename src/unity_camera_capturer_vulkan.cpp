#include "unity_camera_capturer.h"

// unity
#include "unity/IUnityGraphicsVulkan.h"

namespace sora_unity_sdk {

UnityCameraCapturer::VulkanImpl::~VulkanImpl() {
  UnityVulkanInstance instance =
      context_->GetInterfaces()->Get<IUnityGraphicsVulkan>()->Instance();
  VkDevice device = instance.device;

  if (pool_ != VK_NULL_HANDLE) {
    vkDestroyCommandPool(device, pool_, nullptr);
  }
  if (memory_ != VK_NULL_HANDLE) {
    vkFreeMemory(device, memory_, nullptr);
  }
  if (image_ != VK_NULL_HANDLE) {
    vkDestroyImage(device, image_, nullptr);
  }
}

bool UnityCameraCapturer::VulkanImpl::Init(UnityContext* context,
                                           void* camera_texture,
                                           int width,
                                           int height) {
  context_ = context;
  camera_texture_ = camera_texture;
  width_ = width;
  height_ = height;

  UnityVulkanInstance instance =
      context->GetInterfaces()->Get<IUnityGraphicsVulkan>()->Instance();

  VkDevice device = instance.device;
  VkPhysicalDevice physical_device = instance.physicalDevice;
  VkQueue queue = instance.graphicsQueue;
  uint32_t queue_family_index = instance.queueFamilyIndex;

  VkImageCreateInfo imageInfo = {};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.pNext = nullptr;
  imageInfo.flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.format = VK_FORMAT_R8G8B8A8_UINT;
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.tiling = VK_IMAGE_TILING_LINEAR;  // VK_IMAGE_TILING_OPTIMAL;
  imageInfo.usage =
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.queueFamilyIndexCount = 1;
  imageInfo.pQueueFamilyIndices = &queue_family_index;
  if (vkCreateImage(device, &imageInfo, nullptr, &image_) != VK_SUCCESS) {
    RTC_LOG(LS_ERROR) << "vkCreateImage failed";
    return false;
  }

  VkMemoryRequirements mem_requirements;
  vkGetImageMemoryRequirements(device, image_, &mem_requirements);
  RTC_LOG(LS_INFO) << "memoryTypeBits=" << mem_requirements.memoryTypeBits;

  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = mem_requirements.size;

  bool found = false;
  VkPhysicalDeviceMemoryProperties mem_properties;
  vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);

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
  if (!found) {
    RTC_LOG(LS_ERROR) << "VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT not found";
    return false;
  }

  if (vkAllocateMemory(device, &allocInfo, nullptr, &memory_) != VK_SUCCESS) {
    RTC_LOG(LS_ERROR) << "vkAllocateMemory failed";
    return false;
  }

  if (vkBindImageMemory(device, image_, memory_, 0) != VK_SUCCESS) {
    RTC_LOG(LS_ERROR) << "vkBindImageMemory failed";
    return false;
  }

  VkCommandPoolCreateInfo cmdPoolInfo = {};
  cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cmdPoolInfo.queueFamilyIndex = queue_family_index;
  cmdPoolInfo.flags = 0;
  if (vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &pool_) !=
      VK_SUCCESS) {
    RTC_LOG(LS_ERROR) << "vkCreateCommandPool failed";
    return false;
  }

  return true;
}

webrtc::scoped_refptr<webrtc::I420Buffer>
UnityCameraCapturer::VulkanImpl::Capture() {
  IUnityGraphicsVulkan* graphics =
      context_->GetInterfaces()->Get<IUnityGraphicsVulkan>();

  UnityVulkanInstance instance = graphics->Instance();
  VkDevice device = instance.device;
  VkQueue queue = instance.graphicsQueue;

  UnityVulkanImage image;
  bool result = graphics->AccessTexture(
      camera_texture_, UnityVulkanWholeImage,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_ACCESS_TRANSFER_READ_BIT, kUnityVulkanResourceAccess_PipelineBarrier,
      &image);
  if (!result) {
    RTC_LOG(LS_ERROR) << "IUnityGraphicsVulkan::AccessTexture Failed";
    return nullptr;
  }

  VkCommandBuffer command_buffer;

  {
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = pool_;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device, &allocInfo, &command_buffer) !=
        VK_SUCCESS) {
      RTC_LOG(LS_ERROR) << "vkAllocateCommandBuffers failed";
      return nullptr;
    }

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(command_buffer, &beginInfo) != VK_SUCCESS) {
      vkFreeCommandBuffers(device, pool_, 1, &command_buffer);
      RTC_LOG(LS_ERROR) << "vkBeginCommandBuffer failed";
      return nullptr;
    }
  }

  //{
  //  VkImageMemoryBarrier barrier = {};
  //  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  //  barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  //  barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  //  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  //  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

  //  barrier.image = image.image;
  //  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  //  barrier.subresourceRange.baseMipLevel = 0;
  //  barrier.subresourceRange.levelCount = 1;
  //  barrier.subresourceRange.baseArrayLayer = 0;
  //  barrier.subresourceRange.layerCount = 1;

  //  barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  //  barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

  //  vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
  //                       VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
  //                       nullptr, 1, &barrier);
  //}

  //{
  //  VkImageMemoryBarrier barrier = {};
  //  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  //  barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  //  barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  //  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  //  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

  //  barrier.image = image_;
  //  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  //  barrier.subresourceRange.baseMipLevel = 0;
  //  barrier.subresourceRange.levelCount = 1;
  //  barrier.subresourceRange.baseArrayLayer = 0;
  //  barrier.subresourceRange.layerCount = 1;

  //  barrier.srcAccessMask = 0;
  //  barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

  //  vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
  //                       VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
  //                       nullptr, 1, &barrier);
  //}

  VkImageCopy copyRegion{};
  copyRegion.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
  copyRegion.srcOffset = {0, 0, 0};
  copyRegion.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
  copyRegion.dstOffset = {0, 0, 0};
  copyRegion.extent = {(uint32_t)width_, (uint32_t)height_, 1};
  vkCmdCopyImage(command_buffer, image.image,
                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image_,
                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

  //{
  //  VkImageMemoryBarrier barrier = {};
  //  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  //  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  //  barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
  //  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  //  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

  //  barrier.image = image_;
  //  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  //  barrier.subresourceRange.baseMipLevel = 0;
  //  barrier.subresourceRange.levelCount = 1;
  //  barrier.subresourceRange.baseArrayLayer = 0;
  //  barrier.subresourceRange.layerCount = 1;

  //  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  //  barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

  //  vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
  //                       VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
  //                       nullptr, 1, &barrier);
  //}

  //{
  //  VkImageMemoryBarrier barrier = {};
  //  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  //  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  //  barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  //  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  //  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

  //  barrier.image = image.image;
  //  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  //  barrier.subresourceRange.baseMipLevel = 0;
  //  barrier.subresourceRange.levelCount = 1;
  //  barrier.subresourceRange.baseArrayLayer = 0;
  //  barrier.subresourceRange.layerCount = 1;

  //  barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  //  barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

  //  vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
  //                       VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
  //                       nullptr, 1, &barrier);
  //}

  {
    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
      vkFreeCommandBuffers(device, pool_, 1, &command_buffer);
      RTC_LOG(LS_ERROR) << "vkEndCommandBuffer failed";
      return nullptr;
    }

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &command_buffer;

    if (vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
      vkFreeCommandBuffers(device, pool_, 1, &command_buffer);
      RTC_LOG(LS_ERROR) << "vkQueueSubmit failed";
      return nullptr;
    }
    if (vkQueueWaitIdle(queue) != VK_SUCCESS) {
      vkFreeCommandBuffers(device, pool_, 1, &command_buffer);
      RTC_LOG(LS_ERROR) << "vkQueueWaitIdle failed";
      return nullptr;
    }

    vkFreeCommandBuffers(device, pool_, 1, &command_buffer);
  }

  //UnityVulkanImage image;
  ////bool result = graphics->AccessTexture(
  ////    image_, UnityVulkanWholeImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
  ////    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT,
  ////    kUnityVulkanResourceAccess_PipelineBarrier, &image);
  //bool result = graphics->AccessTexture(
  //    image_, UnityVulkanWholeImage, VK_IMAGE_LAYOUT_GENERAL,
  //    VK_PIPELINE_STAGE_HOST_BIT,
  //    VK_ACCESS_HOST_READ_BIT | VK_ACCESS_MEMORY_READ_BIT,
  //    kUnityVulkanResourceAccess_PipelineBarrier, &image);
  //if (!result) {
  //  RTC_LOG(LS_ERROR) << "IUnityGraphicsVulkan::AccessTexture Failed";
  //  return nullptr;
  //}
  //RTC_LOG(LS_INFO) << "memory.memory=" << image.memory.memory;
  //RTC_LOG(LS_INFO) << "memory.offset=" << image.memory.offset;
  //RTC_LOG(LS_INFO) << "memory.size=" << image.memory.size;
  //RTC_LOG(LS_INFO) << "memory.mapped=" << image.memory.mapped;
  //RTC_LOG(LS_INFO) << "memory.flags=" << image.memory.flags;
  //RTC_LOG(LS_INFO) << "memory.memoryTypeIndex=" << image.memory.memoryTypeIndex;
  //RTC_LOG(LS_INFO) << "image=" << image.image;
  //RTC_LOG(LS_INFO) << "layout=" << image.layout;
  //RTC_LOG(LS_INFO) << "aspect=" << image.aspect;
  //RTC_LOG(LS_INFO) << "usage=" << image.usage;
  //RTC_LOG(LS_INFO) << "format=" << image.format;
  //RTC_LOG(LS_INFO) << "extent.width=" << image.extent.width;
  //RTC_LOG(LS_INFO) << "extent.height=" << image.extent.height;
  //RTC_LOG(LS_INFO) << "extent.depth=" << image.extent.depth;
  //RTC_LOG(LS_INFO) << "tiling=" << image.tiling;
  //RTC_LOG(LS_INFO) << "type=" << image.type;
  //RTC_LOG(LS_INFO) << "samples=" << image.samples;
  //// memory.flags == 0b1 // VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
  //// image.usage == 0b10010111 // VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
  //// image.format == 41 // VK_FORMAT_R8G8B8A8_UINT
  //// image.format == 44 // VK_FORMAT_B8G8R8A8_UNORM
  //if (image.memory.mapped == nullptr) {
  //  return nullptr;
  //}
  //if (image.format != VK_FORMAT_R8G8B8A8_UINT) {
  //  return nullptr;
  //}
  //uint8_t* data = (uint8_t*)image.memory.mapped;
  //int pitch = image.extent.width * 4;

  VkImageSubresource subresource{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0};
  VkSubresourceLayout subresource_layout;
  vkGetImageSubresourceLayout(device, image_, &subresource,
                              &subresource_layout);

  uint8_t* data;
  if (vkMapMemory(device, memory_, 0, VK_WHOLE_SIZE, 0, (void**)&data) !=
      VK_SUCCESS) {
    RTC_LOG(LS_ERROR) << "vkMapMemory failed";
    return nullptr;
  }
  int pitch = subresource_layout.rowPitch;

  // Vulkan の場合は座標系の関係で上下反転してるので、頑張って元の向きに戻す
  std::unique_ptr<uint8_t[]> buf(new uint8_t[pitch * height_]);
  for (int i = 0; i < height_; i++) {
    std::memcpy(buf.get() + pitch * i, data + pitch * (height_ - i - 1), pitch);
  }

  vkUnmapMemory(device, memory_);

  webrtc::scoped_refptr<webrtc::I420Buffer> i420_buffer =
      webrtc::I420Buffer::Create(width_, height_);
  libyuv::ARGBToI420(buf.get(), pitch, i420_buffer->MutableDataY(),
                     i420_buffer->StrideY(), i420_buffer->MutableDataU(),
                     i420_buffer->StrideU(), i420_buffer->MutableDataV(),
                     i420_buffer->StrideV(), width_, height_);

  return i420_buffer;
}

}  // namespace sora_unity_sdk
