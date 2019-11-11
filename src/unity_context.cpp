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
  ifs_ = ifs;
  OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);
}

void UnityContext::Shutdown() {
  std::lock_guard<std::mutex> guard(mutex_);
  OnGraphicsDeviceEvent(kUnityGfxDeviceEventShutdown);
  ifs_ = nullptr;
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
