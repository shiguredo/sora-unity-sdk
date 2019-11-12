#include "unity_context.h"

namespace sora {

void UnityContext::OnGraphicsDeviceEventStatic(
    UnityGfxDeviceEventType eventType) {
  Instance().OnGraphicsDeviceEvent(eventType);
}

void UnityContext::OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType) {
  switch (eventType) {
    case kUnityGfxDeviceEventInitialize: {
      graphics_ = ifs_->Get<IUnityGraphics>();
      auto renderer_type = graphics_->GetRenderer();
      if (renderer_type == kUnityGfxRendererD3D11) {
#ifdef _WIN32
        device_ = ifs_->Get<IUnityGraphicsD3D11>()->GetDevice();
        device_->GetImmediateContext(&context_);
#endif
      }
      break;
    }
    case kUnityGfxDeviceEventShutdown:
#ifdef _WIN32
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
#ifdef _WIN32
  return ifs_ != nullptr && device_ != nullptr;
#else
  return ifs_ != nullptr;
#endif
}

void UnityContext::Init(IUnityInterfaces* ifs) {
  std::lock_guard<std::mutex> guard(mutex_);

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

  rtc::LogMessage::AddLogToStream(log_sink_.get(), rtc::LS_INFO);

  RTC_LOG(LS_INFO) << "Log initialized";

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

#ifdef _WIN32
ID3D11Device* UnityContext::GetDevice() {
  std::lock_guard<std::mutex> guard(mutex_);
  return device_;
}

ID3D11DeviceContext* UnityContext::GetDeviceContext() {
  std::lock_guard<std::mutex> guard(mutex_);
  return context_;
}
#endif

}  // namespace sora
