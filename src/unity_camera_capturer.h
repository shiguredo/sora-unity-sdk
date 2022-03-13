#ifndef SORA_UNITY_CAMERA_CAPTURER_H_INCLUDED
#define SORA_UNITY_CAMERA_CAPTURER_H_INCLUDED

// WebRTC
#include "api/media_stream_interface.h"
#include "api/scoped_refptr.h"
#include "api/video/i420_buffer.h"
#include "libyuv.h"
#include "rtc_base/logging.h"
#include "rtc_base/ref_counted_object.h"
#include "system_wrappers/include/clock.h"

// sora
#include "rtc/scalable_track_source.h"
#include "unity_context.h"

#ifdef SORA_UNITY_SDK_ANDROID
#include <vulkan/vulkan.h>
#endif

namespace sora {

class UnityCameraCapturer : public sora::ScalableVideoTrackSource,
                            public rtc::VideoSinkInterface<webrtc::VideoFrame> {
  webrtc::Clock* clock_ = webrtc::Clock::GetRealTimeClock();

#ifdef SORA_UNITY_SDK_WINDOWS
  class D3D11Impl {
    UnityContext* context_;
    void* camera_texture_;
    void* frame_texture_;
    int width_;
    int height_;

   public:
    bool Init(UnityContext* context,
              void* camera_texture,
              int width,
              int height);
    rtc::scoped_refptr<webrtc::I420Buffer> Capture();
  };
  std::unique_ptr<D3D11Impl> capturer_;
#endif

#ifdef SORA_UNITY_SDK_HOLOLENS2
  class NullImpl {
    int width_;
    int height_;

   public:
    bool Init(UnityContext* context,
              void* camera_texture,
              int width,
              int height);
    rtc::scoped_refptr<webrtc::I420Buffer> Capture();
  };
  std::unique_ptr<NullImpl> capturer_;
#endif

#if defined(SORA_UNITY_SDK_MACOS) || defined(SORA_UNITY_SDK_IOS)
  class MetalImpl {
    UnityContext* context_;
    void* camera_texture_;
    void* frame_texture_;
    int width_;
    int height_;

   public:
    bool Init(UnityContext* context,
              void* camera_texture,
              int width,
              int height);
    rtc::scoped_refptr<webrtc::I420Buffer> Capture();
  };
  std::unique_ptr<MetalImpl> capturer_;
#endif

#ifdef SORA_UNITY_SDK_ANDROID
  class VulkanImpl {
    UnityContext* context_;
    void* camera_texture_;
    VkImage image_ = VK_NULL_HANDLE;
    VkDeviceMemory memory_ = VK_NULL_HANDLE;
    VkCommandPool pool_ = VK_NULL_HANDLE;
    int width_;
    int height_;

   public:
    ~VulkanImpl();
    bool Init(UnityContext* context,
              void* camera_texture,
              int width,
              int height);
    rtc::scoped_refptr<webrtc::I420Buffer> Capture();
  };
  std::unique_ptr<VulkanImpl> capturer_;
#endif

 public:
  static rtc::scoped_refptr<UnityCameraCapturer> Create(
      UnityContext* context,
      void* unity_camera_texture,
      int width,
      int height);

  void OnRender();

  void OnFrame(const webrtc::VideoFrame& frame) override;

 private:
  bool Init(UnityContext* context,
            void* unity_camera_texture,
            int width,
            int height);
};

}  // namespace sora

#endif  // SORA_UNITY_CAMERA_CAPTURER_H_INCLUDED
