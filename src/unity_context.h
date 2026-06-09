#ifndef SORA_UNITY_SDK_UNITY_CONTEXT_H_INCLUDED
#define SORA_UNITY_SDK_UNITY_CONTEXT_H_INCLUDED

#include <mutex>

// webrtc
#include "rtc_base/log_sinks.h"

#include "unity/IUnityGraphics.h"
#include "unity/IUnityInterface.h"

#ifdef SORA_UNITY_SDK_WINDOWS
#include <d3d11.h>
#include <d3d12.h>
#include "unity/IUnityGraphicsD3D11.h"
#include "unity/IUnityGraphicsD3D12.h"
#endif

namespace sora_unity_sdk {

#if defined(SORA_UNITY_SDK_MACOS)
// macOS 環境の Unity Editor のみで発生する問題への対処:
// - ビルドバイナリでは再現しない
// - 他のプラットフォーム (Windows / Linux / Android / iOS) では再現しない
// ~Sora() 内で直接 io_context を破棄すると Editor の実行ループと競合するため、
// io_context の破棄を UnityPluginUnload まで延期することで回避する。
void ProcessDeferredIocCleanup();
#endif

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
  ID3D12Device* d3d12_device_ = nullptr;
  ID3D12CommandQueue* d3d12_command_queue_ = nullptr;

 public:
  ID3D11Device* GetD3D11Device();
  ID3D11DeviceContext* GetD3D11DeviceContext();
  ID3D12Device* GetD3D12Device();
  ID3D12CommandQueue* GetD3D12CommandQueue();
#endif
};

std::string UnityGfxRendererToString(UnityGfxRenderer renderer);

}  // namespace sora_unity_sdk

#endif
