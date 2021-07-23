#include "unity.h"
#include "rtc/device_list.h"
#include "sora.h"
#include "sora_conf.json.h"
#include "sora_conf_internal.json.h"

#if defined(SORA_UNITY_SDK_WINDOWS)
#include "hwenc_nvcodec/nvcodec_h264_encoder.h"
#include "hwenc_nvcodec/nvcodec_video_decoder.h"
#endif

extern "C" {

#if defined(SORA_UNITY_SDK_IOS)
// PlaybackEngines/iOSSupport/Trampoline/Classes/Unity/UnityInterface.h から必要な定義だけ拾ってきた
typedef void (*UnityPluginLoadFunc)(IUnityInterfaces* unityInterfaces);
typedef void (*UnityPluginUnloadFunc)();
void UnityRegisterRenderingPluginV5(UnityPluginLoadFunc loadPlugin,
                                    UnityPluginUnloadFunc unloadPlugin);

bool g_ios_plugin_registered = false;

void UNITY_INTERFACE_API SoraUnitySdk_UnityPluginLoad(IUnityInterfaces* ifs);
void UNITY_INTERFACE_API SoraUnitySdk_UnityPluginUnload();
#endif

void* sora_create() {
#if defined(SORA_UNITY_SDK_IOS)
  if (!g_ios_plugin_registered) {
    UnityRegisterRenderingPluginV5(&SoraUnitySdk_UnityPluginLoad,
                                   &SoraUnitySdk_UnityPluginUnload);
    g_ios_plugin_registered = true;
  }
#endif

  auto context = &sora::UnityContext::Instance();
  if (!context->IsInitialized()) {
    return nullptr;
  }

  auto sora = std::unique_ptr<sora::Sora>(new sora::Sora(context));
  return sora.release();
}

void sora_set_on_add_track(void* p, track_cb_t on_add_track, void* userdata) {
  auto sora = (sora::Sora*)p;
  sora->SetOnAddTrack([on_add_track, userdata](ptrid_t track_id) {
    on_add_track(track_id, userdata);
  });
}

void sora_set_on_remove_track(void* p,
                              track_cb_t on_remove_track,
                              void* userdata) {
  auto sora = (sora::Sora*)p;
  sora->SetOnRemoveTrack([on_remove_track, userdata](ptrid_t track_id) {
    on_remove_track(track_id, userdata);
  });
}

void sora_set_on_notify(void* p, notify_cb_t on_notify, void* userdata) {
  auto sora = (sora::Sora*)p;
  sora->SetOnNotify([on_notify, userdata](std::string json) {
    on_notify(json.c_str(), userdata);
  });
}

void sora_set_on_push(void* p, push_cb_t on_push, void* userdata) {
  auto sora = (sora::Sora*)p;
  sora->SetOnPush([on_push, userdata](std::string json) {
    on_push(json.c_str(), userdata);
  });
}

void sora_set_on_message(void* p, message_cb_t on_message, void* userdata) {
  auto sora = (sora::Sora*)p;
  sora->SetOnMessage(
      [on_message, userdata](std::string label, std::string data) {
        on_message(label.c_str(), data.c_str(), (int)data.size(), userdata);
      });
}

void sora_set_on_disconnect(void* p,
                            disconnect_cb_t on_disconnect,
                            void* userdata) {
  auto sora = (sora::Sora*)p;
  sora->SetOnDisconnect(
      [on_disconnect, userdata](int error_code, std::string reason) {
        on_disconnect(error_code, reason.c_str(), userdata);
      });
}

void sora_dispatch_events(void* p) {
  auto sora = (sora::Sora*)p;
  sora->DispatchEvents();
}

void sora_connect(void* p, const char* config_json) {
  auto sora = (sora::Sora*)p;
  auto config =
      jsonif::from_json<sora_conf::internal::ConnectConfig>(config_json);
  sora->Connect(config);
}

void sora_disconnect(void* p) {
  auto sora = (sora::Sora*)p;
  sora->Disconnect();
}

void* sora_get_texture_update_callback() {
  return (void*)&sora::UnityRenderer::Sink::TextureUpdateCallback;
}

void sora_destroy(void* sora) {
  delete (sora::Sora*)sora;
}

void* sora_get_render_callback() {
  return (void*)&sora::Sora::RenderCallbackStatic;
}
int sora_get_render_callback_event_id(void* p) {
  auto sora = (sora::Sora*)p;
  return sora->GetRenderCallbackEventID();
}

void sora_process_audio(void* p, const void* buf, int offset, int samples) {
  auto sora = (sora::Sora*)p;
  sora->ProcessAudio(buf, offset, samples);
}
void sora_set_on_handle_audio(void* p, handle_audio_cb_t f, void* userdata) {
  auto sora = (sora::Sora*)p;
  sora->SetOnHandleAudio(
      [f, userdata](const int16_t* buf, int samples, int channels) {
        f(buf, samples, channels, userdata);
      });
}

void sora_get_stats(void* p, stats_cb_t f, void* userdata) {
  auto sora = (sora::Sora*)p;
  sora->GetStats(
      [f, userdata](std::string json) { f(json.c_str(), userdata); });
}

void sora_send_message(void* p, const char* label, void* buf, int size) {
  auto sora = (sora::Sora*)p;
  const char* s = (const char*)buf;
  sora->SendMessage(label, std::string(s, s + size));
}

unity_bool_t sora_device_enum_video_capturer(device_enum_cb_t f,
                                             void* userdata) {
  return sora::DeviceList::EnumVideoCapturer(
      [f, userdata](std::string device_name, std::string unique_name) {
        f(device_name.c_str(), unique_name.c_str(), userdata);
      });
}
unity_bool_t sora_device_enum_audio_recording(device_enum_cb_t f,
                                              void* userdata) {
  return sora::DeviceList::EnumAudioRecording(
      [f, userdata](std::string device_name, std::string unique_name) {
        f(device_name.c_str(), unique_name.c_str(), userdata);
      });
}
unity_bool_t sora_device_enum_audio_playout(device_enum_cb_t f,
                                            void* userdata) {
  return sora::DeviceList::EnumAudioPlayout(
      [f, userdata](std::string device_name, std::string unique_name) {
        f(device_name.c_str(), unique_name.c_str(), userdata);
      });
}

unity_bool_t sora_is_h264_supported() {
#if defined(SORA_UNITY_SDK_WINDOWS)
  return NvCodecH264Encoder::IsSupported() &&
         NvCodecVideoDecoder::IsSupported(cudaVideoCodec_H264);
#elif defined(SORA_UNITY_SDK_MACOS) || defined(SORA_UNITY_SDK_IOS)
  // macOS, iOS は VideoToolbox が使えるので常に true
  return true;
#elif defined(SORA_UNITY_SDK_ANDROID)
  // Android は多分大体動くので true
  return true;
#endif
}

// iOS の場合は static link で名前が被る可能性があるので、別の名前にしておく
void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
#if defined(SORA_UNITY_SDK_IOS)
SoraUnitySdk_UnityPluginLoad(IUnityInterfaces* ifs)
#else
UnityPluginLoad(IUnityInterfaces* ifs)
#endif
{
  sora::UnityContext::Instance().Init(ifs);
}

void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
#if defined(SORA_UNITY_SDK_IOS)
SoraUnitySdk_UnityPluginUnload()
#else
UnityPluginUnload()
#endif
{
  sora::UnityContext::Instance().Shutdown();
}
}
