#ifndef SORA_UNITY_CONTEXT_H_INCLUDED
#define SORA_UNITY_CONTEXT_H_INCLUDED

#include <mutex>

// webrtc
#include "rtc_base/log_sinks.h"

#include "unity/IUnityGraphics.h"
#include "unity/IUnityInterface.h"

#ifdef _WIN32
#include "unity/IUnityGraphicsD3D11.h"
#endif

namespace sora {

class UnityContext {
  std::mutex mutex_;
  std::unique_ptr<rtc::FileRotatingLogSink> log_sink_;
  IUnityInterfaces* ifs_ = nullptr;
  IUnityGraphics* graphics_ = nullptr;
#ifdef _WIN32
  ID3D11Device* device_ = nullptr;
  ID3D11DeviceContext* context_ = nullptr;
#endif

 private:
  // Unity のプラグインイベント
  static void UNITY_INTERFACE_API
  OnGraphicsDeviceEventStatic(UnityGfxDeviceEventType eventType);

  void UNITY_INTERFACE_API
  OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType);

 public:
  static UnityContext& Instance();

  bool IsInitialized();
  void Init(IUnityInterfaces* ifs);
  void Shutdown();

#ifdef _WIN32
  ID3D11Device* GetDevice();
  ID3D11DeviceContext* GetDeviceContext();
#endif
};

}  // namespace sora

#endif  // SORA_UNITY_CONTEXT_H_INCLUDED
