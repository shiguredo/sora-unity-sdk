#ifndef SORA_UNITY_SDK_UNITY_CONTEXT_H_INCLUDED
#define SORA_UNITY_SDK_UNITY_CONTEXT_H_INCLUDED

#include <mutex>

// webrtc
#include "rtc_base/log_sinks.h"

#include "unity/IUnityGraphics.h"
#include "unity/IUnityInterface.h"

#ifdef SORA_UNITY_SDK_WINDOWS
#include <d3d11.h>
#include "unity/IUnityGraphicsD3D11.h"
#endif

namespace sora_unity_sdk {

class UnityContext {
  std::mutex mutex_;
  std::unique_ptr<webrtc::FileRotatingLogSink> log_sink_;
  IUnityInterfaces* ifs_ = nullptr;
  IUnityGraphics* graphics_ = nullptr;

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

  IUnityInterfaces* GetInterfaces();

#ifdef SORA_UNITY_SDK_WINDOWS
 private:
  ID3D11Device* d3d11_device_ = nullptr;
  ID3D11DeviceContext* d3d11_device_context_ = nullptr;

 public:
  ID3D11Device* GetD3D11Device();
  ID3D11DeviceContext* GetD3D11DeviceContext();
#endif
};

}  // namespace sora_unity_sdk

#endif
