#include "unity_camera_capturer.h"

namespace sora_unity_sdk {

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
    defined(SORA_UNITY_SDK_IOS) || defined(SORA_UNITY_SDK_ANDROID) ||   \
    defined(SORA_UNITY_SDK_UBUNTU)
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
  capturer_.reset();

  auto renderer_type =
      context->GetInterfaces()->Get<IUnityGraphics>()->GetRenderer();

  switch (renderer_type) {
    case kUnityGfxRendererD3D11:
#ifdef SORA_UNITY_SDK_WINDOWS
      RTC_LOG(LS_INFO) << "Init UnityCameraCapturer with D3D11Impl";
      capturer_.reset(new D3D11Impl());
#endif

      break;
    case kUnityGfxRendererMetal:
#if defined(SORA_UNITY_SDK_MACOS) || defined(SORA_UNITY_SDK_IOS)
      RTC_LOG(LS_INFO) << "Init UnityCameraCapturer with MetalImpl";
      capturer_.reset(new MetalImpl());
#endif
      break;
    case kUnityGfxRendererVulkan:
#ifdef SORA_UNITY_SDK_ANDROID
      RTC_LOG(LS_INFO) << "Init UnityCameraCapturer with VulkanImpl";
      capturer_.reset(new VulkanImpl());
#endif
      break;
    case kUnityGfxRendererOpenGLCore:
    case kUnityGfxRendererOpenGLES20:
    case kUnityGfxRendererOpenGLES30:
#if defined(SORA_UNITY_SDK_ANDROID) || defined(SORA_UNITY_SDK_UBUNTU)
      RTC_LOG(LS_INFO) << "Init UnityCameraCapturer with OpenglImpl";
      capturer_.reset(new OpenglImpl());
#endif
      break;
    default:
#ifdef SORA_UNITY_SDK_HOLOLENS2
      RTC_LOG(LS_INFO) << "Init UnityCameraCapturer with NullImpl";
      capturer_.reset(new NullImpl());
#endif

      break;
  }

  if (capturer_ == nullptr) {
    RTC_LOG(LS_INFO) << "Failed to Init for UnityCameraCapturer";
    return false;
  }

  if (!capturer_->Init(context, unity_camera_texture, width, height)) {
    return false;
  }
  return true;
}

}  // namespace sora_unity_sdk
