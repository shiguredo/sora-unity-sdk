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
    case kUnityGfxRendererGameCoreXboxOne:
      return "kUnityGfxRendererGameCoreXboxOne";
    case kUnityGfxRendererGameCoreXboxSeries:
      return "kUnityGfxRendererGameCoreXboxSeries";
    case kUnityGfxRendererPS5:
      return "kUnityGfxRendererPS5";
    case kUnityGfxRendererPS5NGGC:
      return "kUnityGfxRendererPS5NGGC";
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
        d3d11_device_ = ifs_->Get<IUnityGraphicsD3D11>()->GetDevice();
        d3d11_device_->GetImmediateContext(&d3d11_device_context_);
      }
      if (renderer_type == kUnityGfxRendererD3D12) {
        d3d12_device_ = ifs_->Get<IUnityGraphicsD3D12v4>()->GetDevice();
        d3d12_command_queue_ =
            ifs_->Get<IUnityGraphicsD3D12v4>()->GetCommandQueue();
      }
#endif
      break;
    }
    case kUnityGfxDeviceEventShutdown:
#ifdef SORA_UNITY_SDK_WINDOWS
      if (d3d11_device_context_ != nullptr) {
        d3d11_device_context_->Release();
        d3d11_device_context_ = nullptr;
      }
      d3d11_device_ = nullptr;
      d3d12_device_ = nullptr;
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
  if (ifs_ == nullptr || graphics_ == nullptr) {
    return false;
  }
  // kUnityGfxDeviceEventInitialize さえ呼ばれていれば初期化済みとみなす。
  // 未対応のレンダリングタイプだった場合は Unity カメラキャプチャが動作しないが
  // それ以外の機能は使えるので許容する。
  return true;
}

void UnityContext::Init(IUnityInterfaces* ifs) {
  std::lock_guard<std::mutex> guard(mutex_);

#if defined(SORA_UNITY_SDK_WINDOWS) || defined(SORA_UNITY_SDK_MACOS) || \
    defined(SORA_UNITY_SDK_UBUNTU)
  const size_t kDefaultMaxLogFileSize = 10 * 1024 * 1024;
  webrtc::LogMessage::LogToDebug((webrtc::LoggingSeverity)webrtc::LS_NONE);
  webrtc::LogMessage::LogTimestamps();
  webrtc::LogMessage::LogThreads();

  log_sink_.reset(new webrtc::FileRotatingLogSink("./", "webrtc_logs",
                                                  kDefaultMaxLogFileSize, 10));
  if (!log_sink_->Init()) {
    RTC_LOG(LS_ERROR) << __FUNCTION__ << ": Failed to open log file";
    log_sink_.reset();
    return;
  }
  log_sink_->DisableBuffering();

  webrtc::LogMessage::AddLogToStream(log_sink_.get(), webrtc::LS_INFO);

  RTC_LOG(LS_INFO) << "Log initialized";
#endif

#if defined(SORA_UNITY_SDK_ANDROID) || defined(SORA_UNITY_SDK_IOS)
  webrtc::LogMessage::LogToDebug((webrtc::LoggingSeverity)webrtc::LS_INFO);
  webrtc::LogMessage::LogTimestamps();
  webrtc::LogMessage::LogThreads();
#endif

  ifs_ = ifs;
  OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);
}

void UnityContext::Shutdown() {
  std::lock_guard<std::mutex> guard(mutex_);
  OnGraphicsDeviceEvent(kUnityGfxDeviceEventShutdown);
  ifs_ = nullptr;

  RTC_LOG(LS_INFO) << "Log uninitialized";

  webrtc::LogMessage::RemoveLogToStream(log_sink_.get());
  log_sink_.reset();
}

IUnityInterfaces* UnityContext::GetInterfaces() {
  return ifs_;
}

#ifdef SORA_UNITY_SDK_WINDOWS
ID3D11Device* UnityContext::GetD3D11Device() {
  std::lock_guard<std::mutex> guard(mutex_);
  return d3d11_device_;
}

ID3D11DeviceContext* UnityContext::GetD3D11DeviceContext() {
  std::lock_guard<std::mutex> guard(mutex_);
  return d3d11_device_context_;
}

ID3D12Device* UnityContext::GetD3D12Device() {
  std::lock_guard<std::mutex> guard(mutex_);
  return d3d12_device_;
}

ID3D12CommandQueue* UnityContext::GetD3D12CommandQueue() {
  std::lock_guard<std::mutex> guard(mutex_);
  return d3d12_command_queue_;
}
#endif

}  // namespace sora_unity_sdk
