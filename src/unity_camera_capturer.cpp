#include "unity_camera_capturer.h"

namespace sora {

rtc::scoped_refptr<UnityCameraCapturer> UnityCameraCapturer::Create(
    UnityContext* context,
    void* unity_camera_texture,
    int width,
    int height) {
  rtc::scoped_refptr<UnityCameraCapturer> p(
      new rtc::RefCountedObject<UnityCameraCapturer>());
  if (!p->Init(context, unity_camera_texture, width, height)) {
    return nullptr;
  }
  return p;
}

void UnityCameraCapturer::OnRender() {
#if defined(SORA_UNITY_SDK_WINDOWS) || defined(SORA_UNITY_SDK_MACOS) || \
    defined(SORA_UNITY_SDK_ANDROID)
  auto i420_buffer = capturer_->Capture();
  if (!i420_buffer) {
    return;
  }

  auto video_frame = webrtc::VideoFrame::Builder()
                         .set_video_frame_buffer(i420_buffer)
                         .set_rotation(webrtc::kVideoRotation_0)
                         .set_timestamp_us(clock_->TimeInMicroseconds())
                         .build();
  this->OnFrame(video_frame);
#endif
}

void UnityCameraCapturer::OnFrame(const webrtc::VideoFrame& frame) {
  OnCapturedFrame(frame);
}

bool UnityCameraCapturer::Init(UnityContext* context,
                               void* unity_camera_texture,
                               int width,
                               int height) {
#ifdef SORA_UNITY_SDK_WINDOWS
  capturer_.reset(new D3D11Impl());
  if (!capturer_->Init(context, unity_camera_texture, width, height)) {
    return false;
  }
#endif

#ifdef SORA_UNITY_SDK_MACOS
  capturer_.reset(new MetalImpl());
  if (!capturer_->Init(context, unity_camera_texture, width, height)) {
    return false;
  }
#endif

#ifdef SORA_UNITY_SDK_ANDROID
  capturer_.reset(new VulkanImpl());
  if (!capturer_->Init(context, unity_camera_texture, width, height)) {
    return false;
  }
#endif

  return true;
}

}  // namespace sora
