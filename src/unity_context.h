#ifndef SORA_UNITY_CONTEXT_H_INCLUDED
#define SORA_UNITY_CONTEXT_H_INCLUDED

#include <mutex>

// webrtc
#include "rtc_base/log_sinks.h"

#include "unity/IUnityGraphics.h"
#include "unity/IUnityInterface.h"

#ifdef SORA_UNITY_SDK_WINDOWS
#include "unity/IUnityGraphicsD3D11.h"
#endif

namespace sora {

#ifdef SORA_UNITY_SDK_HOLOLENS2
class OutputDebugSink : public rtc::LogSink {
  //static void DoUTFConversion(const char* src,
  //                            int32_t src_len,
  //                            char16_t* dest,
  //                            int32_t* dest_len) {
  //  for (int32_t i = 0; i < src_len;) {
  //    int32_t code_point;
  //    CBU8_NEXT(src, i, src_len, code_point);

  //    CBU16_APPEND_UNSAFE(dest, *dest_len, code_point);
  //  }
  //}
  //static std::u16string UTF8ToUTF16(std::string utf8) {
  //  std::u16string ret;

  //  ret.resize(utf8.size() * 3);

  //  auto* dest = &ret[0];

  //  // ICU requires 32 bit numbers.
  //  int32_t src_len32 = static_cast<int32_t>(utf8.length());
  //  int32_t dest_len32 = 0;

  //  DoUTFConversion(utf8.data(), src_len32, dest, &dest_len32);

  //  ret.resize(dest_len32);
  //  ret.shrink_to_fit();

  //  return ret;
  //}

 public:
  void OnLogMessage(const std::string& message) override {
    //OutputDebugStringW(
    //    reinterpret_cast<const wchar_t*>(UTF8ToUTF16(message).c_str()));
    printf(message.c_str());
    fflush(stdout);
  }
};
#endif

class UnityContext {
  std::mutex mutex_;
#ifdef SORA_UNITY_SDK_HOLOLENS2
  std::unique_ptr<OutputDebugSink> log_sink_;
#else
  std::unique_ptr<rtc::FileRotatingLogSink> log_sink_;
#endif
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
  ID3D11Device* device_ = nullptr;
  ID3D11DeviceContext* context_ = nullptr;

 public:
  ID3D11Device* GetDevice();
  ID3D11DeviceContext* GetDeviceContext();
#endif
};

}  // namespace sora

#endif  // SORA_UNITY_CONTEXT_H_INCLUDED
