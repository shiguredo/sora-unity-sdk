#ifndef SORA_UNITY_SDK_UNITY_CAMERA_CAPTURER_H_INCLUDED
#define SORA_UNITY_SDK_UNITY_CAMERA_CAPTURER_H_INCLUDED

// WebRTC
#include <api/media_stream_interface.h>
#include <api/scoped_refptr.h>
#include <api/video/i420_buffer.h>
#include <libyuv.h>
#include <rtc_base/logging.h>
#include <rtc_base/ref_counted_object.h>
#include <system_wrappers/include/clock.h>

// sora
#include <sora/scalable_track_source.h>

#include "unity_context.h"

#ifdef SORA_UNITY_SDK_ANDROID
#include <vulkan/vulkan.h>
#endif

namespace sora_unity_sdk {

struct UnityCameraCapturerConfig : sora::ScalableVideoTrackSourceConfig {
  UnityContext* context;
  void* unity_camera_texture;
  int width;
  int height;
};

class UnityCameraCapturer
    : public sora::ScalableVideoTrackSource,
      public webrtc::VideoSinkInterface<webrtc::VideoFrame> {
  webrtc::Clock* clock_ = webrtc::Clock::GetRealTimeClock();

  struct Impl {
    virtual ~Impl() {}
    virtual bool Init(UnityContext* context,
                      void* camera_texture,
                      int width,
                      int height) = 0;
    virtual webrtc::scoped_refptr<webrtc::I420Buffer> Capture() = 0;
  };

#ifdef SORA_UNITY_SDK_WINDOWS
  class D3D11Impl : public Impl {
    UnityContext* context_;
    void* camera_texture_;
    void* frame_texture_;
    int width_;
    int height_;

   public:
    bool Init(UnityContext* context,
              void* camera_texture,
              int width,
              int height) override;
    webrtc::scoped_refptr<webrtc::I420Buffer> Capture() override;
  };

  class D3D12Impl : public Impl {
    UnityContext* context_ = nullptr;
    ID3D12Resource* camera_texture_ = nullptr;
    int width_ = 0;
    int height_ = 0;

    ID3D12Resource* readback_buffer_ = nullptr;
    size_t readback_buffer_total_bytes_ = 0;
    size_t readback_buffer_row_pitch_ = 0;

    ID3D12CommandAllocator* cmd_allocator_ = nullptr;
    ID3D12GraphicsCommandList* cmd_list_ = nullptr;
    ID3D12Fence* fence_ = nullptr;
    HANDLE fence_event_ = nullptr;
    UINT64 fence_value_ = 0;
    ID3D12CommandQueue* queue_ = nullptr;

   public:
    ~D3D12Impl() override;
    bool Init(UnityContext* context,
              void* camera_texture,
              int width,
              int height) override;
    webrtc::scoped_refptr<webrtc::I420Buffer> Capture() override;
  };
#endif

#if defined(SORA_UNITY_SDK_MACOS) || defined(SORA_UNITY_SDK_IOS)
  class MetalImpl : public Impl {
    UnityContext* context_;
    void* camera_texture_;
    void* frame_texture_;
    int width_;
    int height_;

   public:
    bool Init(UnityContext* context,
              void* camera_texture,
              int width,
              int height) override;
    webrtc::scoped_refptr<webrtc::I420Buffer> Capture() override;
  };
#endif

#ifdef SORA_UNITY_SDK_ANDROID
  class VulkanImpl : public Impl {
    UnityContext* context_;
    void* camera_texture_;
    VkImage image_ = VK_NULL_HANDLE;
    VkDeviceMemory memory_ = VK_NULL_HANDLE;
    VkCommandPool pool_ = VK_NULL_HANDLE;
    int width_;
    int height_;

   public:
    ~VulkanImpl() override;
    bool Init(UnityContext* context,
              void* camera_texture,
              int width,
              int height) override;
    webrtc::scoped_refptr<webrtc::I420Buffer> Capture() override;
  };
#endif

#if defined(SORA_UNITY_SDK_ANDROID) || defined(SORA_UNITY_SDK_UBUNTU)
  class OpenglImpl : public Impl {
    UnityContext* context_;
    void* camera_texture_;
    int width_;
    int height_;
    unsigned int fbo_ = 0;
    bool initialized_ = false;

   public:
    ~OpenglImpl() override;
    bool Init(UnityContext* context,
              void* camera_texture,
              int width,
              int height) override;
    webrtc::scoped_refptr<webrtc::I420Buffer> Capture() override;
  };
#endif

  std::unique_ptr<Impl> capturer_;
  std::mutex mutex_;
  bool stopped_ = false;

 public:
  static webrtc::scoped_refptr<UnityCameraCapturer> Create(
      const UnityCameraCapturerConfig& config);

  UnityCameraCapturer(const UnityCameraCapturerConfig& config);

  void OnRender();

  void Stop();

  void OnFrame(const webrtc::VideoFrame& frame) override;

 private:
  bool Init(UnityContext* context,
            void* unity_camera_texture,
            int width,
            int height);
};

}  // namespace sora_unity_sdk

#endif
