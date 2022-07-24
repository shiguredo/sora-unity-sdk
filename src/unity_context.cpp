#include "unity_context.h"

namespace sora_unity_sdk {

void UnityContext::OnGraphicsDeviceEventStatic(
    UnityGfxDeviceEventType eventType) {
  Instance().OnGraphicsDeviceEvent(eventType);
}

static std::string UnityGfxRendererToString(UnityGfxRenderer renderer) {
  switch (renderer) {
    case kUnityGfxRendererD3D11:
      return "kUnityGfxRendererD3D11";
    case kUnityGfxRendererNull:
      return "kUnityGfxRendererNull";
    case kUnityGfxRendererOpenGLES20:
      return "kUnityGfxRendererOpenGLES20";
    case kUnityGfxRendererOpenGLES30:
      return "kUnityGfxRendererOpenGLES30";
    case kUnityGfxRendererPS4:
      return "kUnityGfxRendererPS4";
    case kUnityGfxRendererXboxOne:
      return "kUnityGfxRendererXboxOne";
    case kUnityGfxRendererMetal:
      return "kUnityGfxRendererMetal";
    case kUnityGfxRendererOpenGLCore:
      return "kUnityGfxRendererOpenGLCore";
    case kUnityGfxRendererD3D12:
      return "kUnityGfxRendererD3D12";
    case kUnityGfxRendererVulkan:
      return "kUnityGfxRendererVulkan";
    case kUnityGfxRendererNvn:
      return "kUnityGfxRendererNvn";
    case kUnityGfxRendererXboxOneD3D12:
      return "kUnityGfxRendererXboxOneD3D12";
    default:
      return "Unknown";
  }
}

void UnityContext::OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType) {
  switch (eventType) {
    case kUnityGfxDeviceEventInitialize: {
      graphics_ = ifs_->Get<IUnityGraphics>();
      auto renderer_type = graphics_->GetRenderer();
      RTC_LOG(LS_INFO) << "Renderer Type is "
                       << UnityGfxRendererToString(renderer_type);
#ifdef SORA_UNITY_SDK_WINDOWS
      if (renderer_type == kUnityGfxRendererD3D11) {
        device_ = ifs_->Get<IUnityGraphicsD3D11>()->GetDevice();
        device_->GetImmediateContext(&context_);
      }
#endif
      break;
    }
    case kUnityGfxDeviceEventShutdown:
#ifdef SORA_UNITY_SDK_WINDOWS
      if (context_ != nullptr) {
        context_->Release();
        context_ = nullptr;
      }
      device_ = nullptr;
#endif

      if (graphics_ != nullptr) {
        graphics_->UnregisterDeviceEventCallback(OnGraphicsDeviceEventStatic);
        graphics_ = nullptr;
      }
      break;
    case kUnityGfxDeviceEventBeforeReset:
      break;
    case kUnityGfxDeviceEventAfterReset:
      break;
  };
}

UnityContext& UnityContext::Instance() {
  static UnityContext instance;
  return instance;
}

bool UnityContext::IsInitialized() {
  std::lock_guard<std::mutex> guard(mutex_);
#ifdef SORA_UNITY_SDK_WINDOWS
  return ifs_ != nullptr && device_ != nullptr;
#endif

#if defined(SORA_UNITY_SDK_MACOS) || defined(SORA_UNITY_SDK_IOS)
  if (ifs_ == nullptr || graphics_ == nullptr) {
    return false;
  }

  // Metal だけ対応する
  auto renderer_type = graphics_->GetRenderer();
  if (renderer_type != kUnityGfxRendererMetal) {
    return false;
  }

  return true;
#endif
  return true;
}

void UnityContext::Init(IUnityInterfaces* ifs) {
  std::lock_guard<std::mutex> guard(mutex_);

#if defined(SORA_UNITY_SDK_WINDOWS) || defined(SORA_UNITY_SDK_MACOS) || \
    defined(SORA_UNITY_SDK_UBUNTU)
  const size_t kDefaultMaxLogFileSize = 10 * 1024 * 1024;
  rtc::LogMessage::LogToDebug((rtc::LoggingSeverity)rtc::LS_NONE);
  rtc::LogMessage::LogTimestamps();
  rtc::LogMessage::LogThreads();

  log_sink_.reset(new rtc::FileRotatingLogSink("./", "webrtc_logs",
                                               kDefaultMaxLogFileSize, 10));
  if (!log_sink_->Init()) {
    RTC_LOG(LS_ERROR) << __FUNCTION__ << ": Failed to open log file";
    log_sink_.reset();
    return;
  }
  log_sink_->DisableBuffering();

  rtc::LogMessage::AddLogToStream(log_sink_.get(), rtc::LS_INFO);

  RTC_LOG(LS_INFO) << "Log initialized";
#endif

#if defined(SORA_UNITY_SDK_ANDROID) || defined(SORA_UNITY_SDK_IOS)
  rtc::LogMessage::LogToDebug((rtc::LoggingSeverity)rtc::LS_INFO);
  rtc::LogMessage::LogTimestamps();
  rtc::LogMessage::LogThreads();
#endif

  ifs_ = ifs;
  OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);
}

void UnityContext::Shutdown() {
  std::lock_guard<std::mutex> guard(mutex_);
  OnGraphicsDeviceEvent(kUnityGfxDeviceEventShutdown);
  ifs_ = nullptr;

  RTC_LOG(LS_INFO) << "Log uninitialized";

  rtc::LogMessage::RemoveLogToStream(log_sink_.get());
  log_sink_.reset();
}

IUnityInterfaces* UnityContext::GetInterfaces() {
  return ifs_;
}

#ifdef SORA_UNITY_SDK_WINDOWS
ID3D11Device* UnityContext::GetDevice() {
  std::lock_guard<std::mutex> guard(mutex_);
  return device_;
}

ID3D11DeviceContext* UnityContext::GetDeviceContext() {
  std::lock_guard<std::mutex> guard(mutex_);
  return context_;
}
#endif

}  // namespace sora_unity_sdk
