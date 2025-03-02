#include "unity.h"

// Sora
#include <sora/audio_output_helper.h>
#include <sora/sora_video_codec.h>

#include "converter.h"
#include "device_list.h"
#include "sora.h"
#include "sora_conf.json.h"
#include "sora_conf_internal.json.h"
#include "unity_context.h"

#if defined(SORA_UNITY_SDK_WINDOWS) || defined(SORA_UNITY_SDK_UBUNTU)
#include <sora/hwenc_nvcodec/nvcodec_video_decoder.h>
#include <sora/hwenc_nvcodec/nvcodec_video_encoder.h>
#endif

struct SoraWrapper {
  std::shared_ptr<sora_unity_sdk::Sora> sora;
};

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

  auto context = &sora_unity_sdk::UnityContext::Instance();
  if (!context->IsInitialized()) {
    return nullptr;
  }

  auto wsora = std::unique_ptr<SoraWrapper>(new SoraWrapper());
  wsora->sora = std::make_shared<sora_unity_sdk::Sora>(context);
  return wsora.release();
}

void sora_set_on_add_track(void* p, track_cb_t on_add_track, void* userdata) {
  auto wsora = (SoraWrapper*)p;
  wsora->sora->SetOnAddTrack(
      [on_add_track, userdata](ptrid_t track_id, std::string connection_id) {
        on_add_track(track_id, connection_id.c_str(), userdata);
      });
}

void sora_set_on_remove_track(void* p,
                              track_cb_t on_remove_track,
                              void* userdata) {
  auto wsora = (SoraWrapper*)p;
  wsora->sora->SetOnRemoveTrack(
      [on_remove_track, userdata](ptrid_t track_id, std::string connection_id) {
        on_remove_track(track_id, connection_id.c_str(), userdata);
      });
}

void sora_set_on_set_offer(void* p,
                           set_offer_cb_t on_set_offer,
                           void* userdata) {
  auto wsora = (SoraWrapper*)p;
  wsora->sora->SetOnSetOffer([on_set_offer, userdata](std::string json) {
    on_set_offer(json.c_str(), userdata);
  });
}

void sora_set_on_notify(void* p, notify_cb_t on_notify, void* userdata) {
  auto wsora = (SoraWrapper*)p;
  wsora->sora->SetOnNotify([on_notify, userdata](std::string json) {
    on_notify(json.c_str(), userdata);
  });
}

void sora_set_on_push(void* p, push_cb_t on_push, void* userdata) {
  auto wsora = (SoraWrapper*)p;
  wsora->sora->SetOnPush([on_push, userdata](std::string json) {
    on_push(json.c_str(), userdata);
  });
}

void sora_set_on_message(void* p, message_cb_t on_message, void* userdata) {
  auto wsora = (SoraWrapper*)p;
  wsora->sora->SetOnMessage(
      [on_message, userdata](std::string label, std::string data) {
        on_message(label.c_str(), data.c_str(), (int)data.size(), userdata);
      });
}

void sora_set_on_disconnect(void* p,
                            disconnect_cb_t on_disconnect,
                            void* userdata) {
  auto wsora = (SoraWrapper*)p;
  wsora->sora->SetOnDisconnect(
      [on_disconnect, userdata](int error_code, std::string reason) {
        on_disconnect(error_code, reason.c_str(), userdata);
      });
}

void sora_set_on_data_channel(void* p,
                              data_channel_cb_t on_data_channel,
                              void* userdata) {
  auto wsora = (SoraWrapper*)p;
  wsora->sora->SetOnDataChannel([on_data_channel, userdata](std::string label) {
    on_data_channel(label.c_str(), userdata);
  });
}

void sora_set_on_capturer_frame(void* p,
                                capturer_frame_cb_t on_capturer_frame,
                                void* userdata) {
  auto wsora = (SoraWrapper*)p;
  wsora->sora->SetOnCapturerFrame(
      [on_capturer_frame, userdata](std::string data) {
        on_capturer_frame(data.c_str(), userdata);
      });
}

void sora_dispatch_events(void* p) {
  auto wsora = (SoraWrapper*)p;
  wsora->sora->DispatchEvents();
}

void sora_connect(void* p, const char* config_json) {
  auto wsora = (SoraWrapper*)p;
  auto config =
      jsonif::from_json<sora_conf::internal::ConnectConfig>(config_json);
  wsora->sora->Connect(config);
}

void sora_disconnect(void* p) {
  auto wsora = (SoraWrapper*)p;
  wsora->sora->Disconnect();
}

void sora_switch_camera(void* p, const char* config_json) {
  auto wsora = (SoraWrapper*)p;
  auto config =
      jsonif::from_json<sora_conf::internal::CameraConfig>(config_json);
  wsora->sora->SwitchCamera(config);
}

void* sora_get_texture_update_callback() {
  return (void*)&sora_unity_sdk::UnityRenderer::Sink::TextureUpdateCallback;
}

void sora_destroy(void* sora) {
  delete (SoraWrapper*)sora;
}

void* sora_get_render_callback() {
  return (void*)&sora_unity_sdk::Sora::RenderCallbackStatic;
}
int sora_get_render_callback_event_id(void* p) {
  auto wsora = (SoraWrapper*)p;
  return wsora->sora->GetRenderCallbackEventID();
}

void sora_process_audio(void* p, const void* buf, int offset, int samples) {
  auto wsora = (SoraWrapper*)p;
  wsora->sora->ProcessAudio(buf, offset, samples);
}
void sora_set_on_handle_audio(void* p, handle_audio_cb_t f, void* userdata) {
  auto wsora = (SoraWrapper*)p;
  wsora->sora->SetOnHandleAudio(
      [f, userdata](const int16_t* buf, int samples, int channels) {
        f(buf, samples, channels, userdata);
      });
}

void sora_get_stats(void* p, stats_cb_t f, void* userdata) {
  auto wsora = (SoraWrapper*)p;
  wsora->sora->GetStats(
      [f, userdata](std::string json) { f(json.c_str(), userdata); });
}

void sora_send_message(void* p, const char* label, void* buf, int size) {
  auto wsora = (SoraWrapper*)p;
  const char* s = (const char*)buf;
  wsora->sora->SendMessage(label, std::string(s, s + size));
}

unity_bool_t sora_device_enum_video_capturer(device_enum_cb_t f,
                                             void* userdata) {
  return sora_unity_sdk::DeviceList::EnumVideoCapturer(
      [f, userdata](std::string device_name, std::string unique_name) {
        f(device_name.c_str(), unique_name.c_str(), userdata);
      });
}
unity_bool_t sora_device_enum_audio_recording(device_enum_cb_t f,
                                              void* userdata) {
  return sora_unity_sdk::DeviceList::EnumAudioRecording(
      [f, userdata](std::string device_name, std::string unique_name) {
        f(device_name.c_str(), unique_name.c_str(), userdata);
      });
}
unity_bool_t sora_device_enum_audio_playout(device_enum_cb_t f,
                                            void* userdata) {
  return sora_unity_sdk::DeviceList::EnumAudioPlayout(
      [f, userdata](std::string device_name, std::string unique_name) {
        f(device_name.c_str(), unique_name.c_str(), userdata);
      });
}

unity_bool_t sora_is_h264_supported() {
#if defined(SORA_UNITY_SDK_WINDOWS) || defined(SORA_UNITY_SDK_UBUNTU)
  auto context = sora::CudaContext::Create();
  auto codec = sora::CudaVideoCodec::H264;
  return sora::NvCodecVideoEncoder::IsSupported(context, codec) &&
         sora::NvCodecVideoDecoder::IsSupported(context, codec);
#elif defined(SORA_UNITY_SDK_MACOS) || defined(SORA_UNITY_SDK_IOS)
  // macOS, iOS は VideoToolbox が使えるので常に true
  return true;
#elif defined(SORA_UNITY_SDK_ANDROID)
  // Android は多分大体動くので true
  return true;
#else
#error "Unknown SDK Type"
#endif
}

void sora_setenv(const char* name, const char* value) {
#if defined(SORA_UNITY_SDK_WINDOWS)
  _putenv_s(name, value);
#else
  setenv(name, value, 1);
#endif
}

unity_bool_t sora_get_audio_enabled(void* p) {
  auto wsora = (SoraWrapper*)p;
  return wsora->sora->GetAudioEnabled();
}
void sora_set_audio_enabled(void* p, unity_bool_t enabled) {
  auto wsora = (SoraWrapper*)p;
  wsora->sora->SetAudioEnabled(enabled);
}
unity_bool_t sora_get_video_enabled(void* p) {
  auto wsora = (SoraWrapper*)p;
  return wsora->sora->GetVideoEnabled();
}
void sora_set_video_enabled(void* p, unity_bool_t enabled) {
  auto wsora = (SoraWrapper*)p;
  wsora->sora->SetVideoEnabled(enabled);
}

// get_*_signaling_url_size() から get_*_signaling_url() までの間に値が変わった場合、
// 落ちることは無いが、文字列が切り詰められる可能性があるので注意
int sora_get_selected_signaling_url_size(void* p) {
  auto wsora = (SoraWrapper*)p;
  return wsora->sora->GetSelectedSignalingURL().size();
}
int sora_get_connected_signaling_url_size(void* p) {
  auto wsora = (SoraWrapper*)p;
  return wsora->sora->GetConnectedSignalingURL().size();
}
void sora_get_selected_signaling_url(void* p, void* buf, int size) {
  auto wsora = (SoraWrapper*)p;
  std::string str = wsora->sora->GetSelectedSignalingURL();
  std::memcpy(buf, str.c_str(), std::min(size, (int)str.size()));
}
void sora_get_connected_signaling_url(void* p, void* buf, int size) {
  auto wsora = (SoraWrapper*)p;
  std::string str = wsora->sora->GetConnectedSignalingURL();
  std::memcpy(buf, str.c_str(), std::min(size, (int)str.size()));
}
static std::optional<sora::VideoCodecCapability> g_video_codec_capability;
int sora_get_video_codec_capability_size(const char* config) {
  auto c = jsonif::from_json<sora_conf::internal::VideoCodecCapabilityConfig>(
      config);
  auto cc = sora_unity_sdk::ConvertToVideoCodecCapabilityConfigWithSession(c);
  auto capability = sora::GetVideoCodecCapability(cc);
  auto vcc = sora_unity_sdk::ConvertToInternalVideoCodecCapability(capability);
  auto result = jsonif::to_json(vcc);
  g_video_codec_capability = capability;
  return (int)result.size();
}
void sora_get_video_codec_capability(const char* config, void* buf, int size) {
  sora::VideoCodecCapability capability;
  // 今の実装は sora_get_video_codec_capability_size → sora_get_video_codec_capability の順で呼ぶのは確定しているので、
  // sora_get_video_codec_capability_size で取得した capability を流用する
  if (g_video_codec_capability) {
    capability = *g_video_codec_capability;
    g_video_codec_capability.reset();
  } else {
    // 普通に調べる
    auto c = jsonif::from_json<sora_conf::internal::VideoCodecCapabilityConfig>(
        config);
    auto cc = sora_unity_sdk::ConvertToVideoCodecCapabilityConfigWithSession(c);
    capability = sora::GetVideoCodecCapability(cc);
  }
  auto vcc = sora_unity_sdk::ConvertToInternalVideoCodecCapability(capability);
  auto result = jsonif::to_json(vcc);
  if (size < (int)result.size()) {
    return;
  }
  std::memcpy(buf, result.c_str(), result.size());
}
bool sora_video_codec_preference_has_implementation(
    const char* self,
    const char* implementation) {
  auto preference = sora_unity_sdk::ConvertToVideoCodecPreference(
      jsonif::from_json<sora_conf::internal::VideoCodecPreference>(self));
  return preference.HasImplementation(
      boost::json::value_to<sora::VideoCodecImplementation>(
          boost::json::value(implementation)));
}
int sora_video_codec_preference_merge_size(const char* self,
                                           const char* preference) {
  auto selfobj = sora_unity_sdk::ConvertToVideoCodecPreference(
      jsonif::from_json<sora_conf::internal::VideoCodecPreference>(self));
  auto preferenceobj = sora_unity_sdk::ConvertToVideoCodecPreference(
      jsonif::from_json<sora_conf::internal::VideoCodecPreference>(preference));
  selfobj.Merge(preferenceobj);
  auto selfinternal =
      sora_unity_sdk::ConvertToInternalVideoCodecPreference(selfobj);
  auto json = jsonif::to_json(selfinternal);
  return (int)json.size();
}
void sora_video_codec_preference_merge(const char* self,
                                       const char* preference,
                                       void* buf,
                                       int size) {
  auto selfobj = sora_unity_sdk::ConvertToVideoCodecPreference(
      jsonif::from_json<sora_conf::internal::VideoCodecPreference>(self));
  auto preferenceobj = sora_unity_sdk::ConvertToVideoCodecPreference(
      jsonif::from_json<sora_conf::internal::VideoCodecPreference>(preference));
  selfobj.Merge(preferenceobj);
  auto selfinternal =
      sora_unity_sdk::ConvertToInternalVideoCodecPreference(selfobj);
  auto json = jsonif::to_json(selfinternal);
  if (size < (int)json.size()) {
    return;
  }
  std::memcpy(buf, json.c_str(), json.size());
}
int sora_create_video_codec_preference_from_implementation_size(
    const char* capability,
    const char* implementation) {
  auto vcc = sora_unity_sdk::ConvertToVideoCodecCapability(
      jsonif::from_json<sora_conf::internal::VideoCodecCapability>(capability));
  auto r = sora::CreateVideoCodecPreferenceFromImplementation(
      vcc, boost::json::value_to<sora::VideoCodecImplementation>(
               boost::json::value(implementation)));
  auto rinternal = sora_unity_sdk::ConvertToInternalVideoCodecPreference(r);
  auto json = jsonif::to_json(rinternal);
  return (int)json.size();
}
void sora_create_video_codec_preference_from_implementation(
    const char* capability,
    const char* implementation,
    void* buf,
    int size) {
  auto vcc = sora_unity_sdk::ConvertToVideoCodecCapability(
      jsonif::from_json<sora_conf::internal::VideoCodecCapability>(capability));
  auto r = sora::CreateVideoCodecPreferenceFromImplementation(
      vcc, boost::json::value_to<sora::VideoCodecImplementation>(
               boost::json::value(implementation)));
  auto rinternal = sora_unity_sdk::ConvertToInternalVideoCodecPreference(r);
  auto json = jsonif::to_json(rinternal);
  if (size < (int)json.size()) {
    return;
  }
  std::memcpy(buf, json.c_str(), json.size());
}

struct AudioOutputHelperImpl : public sora::AudioChangeRouteObserver {
  AudioOutputHelperImpl(change_route_cb_t cb, void* userdata)
      : helper_(sora::CreateAudioOutputHelper(this)),
        cb_(cb),
        userdata_(userdata) {}
  void OnChangeRoute() override { cb_(userdata_); }

  bool IsHandsfree() { return helper_->IsHandsfree(); }
  void SetHandsfree(bool enabled) { helper_->SetHandsfree(enabled); }

 private:
  std::unique_ptr<sora::AudioOutputHelperInterface> helper_;
  change_route_cb_t cb_;
  void* userdata_;
};

void* sora_audio_output_helper_create(change_route_cb_t cb, void* userdata) {
  return new AudioOutputHelperImpl(cb, userdata);
}
void sora_audio_output_helper_destroy(void* p) {
  delete (AudioOutputHelperImpl*)p;
}
unity_bool_t sora_audio_output_helper_is_handsfree(void* p) {
  auto helper = (AudioOutputHelperImpl*)p;
  return helper->IsHandsfree() ? 1 : 0;
}
void sora_audio_output_helper_set_handsfree(void* p, unity_bool_t enabled) {
  auto helper = (AudioOutputHelperImpl*)p;
  helper->SetHandsfree(enabled != 0);
}

// iOS の場合は static link で名前が被る可能性があるので、別の名前にしておく
void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
#if defined(SORA_UNITY_SDK_IOS)
SoraUnitySdk_UnityPluginLoad(IUnityInterfaces* ifs)
#else
UnityPluginLoad(IUnityInterfaces* ifs)
#endif
{
  sora_unity_sdk::UnityContext::Instance().Init(ifs);
}

void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
#if defined(SORA_UNITY_SDK_IOS)
SoraUnitySdk_UnityPluginUnload()
#else
UnityPluginUnload()
#endif
{
  sora_unity_sdk::UnityContext::Instance().Shutdown();
}
}
