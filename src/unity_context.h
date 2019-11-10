#ifndef SORA_UNITY_CONTEXT_H_INCLUDED
#define SORA_UNITY_CONTEXT_H_INCLUDED

#include <mutex>

#include "unity/IUnityGraphics.h"
#include "unity/IUnityGraphicsD3D11.h"
#include "unity/IUnityInterface.h"

namespace sora {

class UnityContext {
  std::mutex mutex_;
  IUnityInterfaces* ifs_ = nullptr;
  IUnityGraphics* graphics_ = nullptr;
  ID3D11Device* device_ = nullptr;
  ID3D11DeviceContext* context_ = nullptr;

 private:
  // Unity のプラグインイベント
  static void UNITY_INTERFACE_API
  OnGraphicsDeviceEventStatic(UnityGfxDeviceEventType eventType) {
    Instance().OnGraphicsDeviceEvent(eventType);
  }

  void UNITY_INTERFACE_API
  OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType) {
    switch (eventType) {
      case kUnityGfxDeviceEventInitialize: {
        graphics_ = ifs_->Get<IUnityGraphics>();
        auto renderer_type = graphics_->GetRenderer();
        if (renderer_type == kUnityGfxRendererD3D11) {
          device_ = ifs_->Get<IUnityGraphicsD3D11>()->GetDevice();
          device_->GetImmediateContext(&context_);
        }
        break;
      }
      case kUnityGfxDeviceEventShutdown:
        if (context_ != nullptr) {
          context_->Release();
          context_ = nullptr;
        }
        device_ = nullptr;

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

 public:
  static UnityContext& Instance() {
    static UnityContext instance;
    return instance;
  }

  bool IsInitialized() {
    std::lock_guard<std::mutex> guard(mutex_);
    return ifs_ != nullptr && device_ != nullptr;
  }

  void Init(IUnityInterfaces* ifs) {
    std::lock_guard<std::mutex> guard(mutex_);
    ifs_ = ifs;
    OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);
  }

  void Shutdown() {
    std::lock_guard<std::mutex> guard(mutex_);
    OnGraphicsDeviceEvent(kUnityGfxDeviceEventShutdown);
    ifs_ = nullptr;
  }

  ID3D11Device* GetDevice() {
    std::lock_guard<std::mutex> guard(mutex_);
    return device_;
  }

  ID3D11DeviceContext* GetDeviceContext() {
    std::lock_guard<std::mutex> guard(mutex_);
    return context_;
  }
};

}  // namespace sora

#endif  // SORA_UNITY_CONTEXT_H_INCLUDED