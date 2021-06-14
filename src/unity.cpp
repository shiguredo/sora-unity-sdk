#include "unity.h"
#include "rtc/device_list.h"
#include "sora.h"

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
    on_notify(json.c_str(), (int)json.size(), userdata);
  });
}

void sora_set_on_push(void* p, push_cb_t on_push, void* userdata) {
  auto sora = (sora::Sora*)p;
  sora->SetOnPush([on_push, userdata](std::string json) {
    on_push(json.c_str(), (int)json.size(), userdata);
  });
}

void sora_dispatch_events(void* p) {
  auto sora = (sora::Sora*)p;
  sora->DispatchEvents();
}

int sora_connect(void* p,
                 const char* unity_version,
                 const char* signaling_url,
                 const char* channel_id,
                 const char* client_id,
                 const char* metadata,
                 const char* role,
                 unity_bool_t multistream,
                 unity_bool_t spotlight,
                 int spotlight_number,
                 const char* spotlight_focus_rid,
                 const char* spotlight_unfocus_rid,
                 unity_bool_t simulcast,
                 int capturer_type,
                 void* unity_camera_texture,
                 const char* video_capturer_device,
                 int video_width,
                 int video_height,
                 const char* video_codec_type,
                 int video_bit_rate,
                 unity_bool_t unity_audio_input,
                 unity_bool_t unity_audio_output,
                 const char* audio_recording_device,
                 const char* audio_playout_device,
                 const char* audio_codec_type,
                 int audio_bit_rate,
                 unity_bool_t enable_data_channel_signaling,
                 unity_bool_t data_channel_signaling,
                 int data_channel_signaling_timeout,
                 unity_bool_t enable_ignore_disconnect_websocket,
                 unity_bool_t ignore_disconnect_websocket,
                 int disconnect_wait_timeout) {
  auto sora = (sora::Sora*)p;
  sora::Sora::ConnectConfig config;
  config.unity_version = unity_version;
  config.signaling_url = signaling_url;
  config.channel_id = channel_id;
  config.client_id = client_id;
  config.metadata = metadata;
  config.role = role;
  config.multistream = multistream;
  config.spotlight = spotlight;
  config.spotlight_number = spotlight_number;
  config.spotlight_focus_rid = spotlight_focus_rid;
  config.spotlight_unfocus_rid = spotlight_unfocus_rid;
  config.simulcast = simulcast;
  config.capturer_type = capturer_type;
  config.unity_camera_texture = unity_camera_texture;
  config.video_capturer_device = video_capturer_device;
  config.video_width = video_width;
  config.video_height = video_height;
  config.video_codec_type = video_codec_type;
  config.video_bit_rate = video_bit_rate;
  config.unity_audio_input = unity_audio_input;
  config.unity_audio_output = unity_audio_output;
  config.audio_recording_device = audio_recording_device;
  config.audio_playout_device = audio_playout_device;
  config.audio_codec_type = audio_codec_type;
  config.audio_bit_rate = audio_bit_rate;
  config.enable_data_channel_signaling = enable_data_channel_signaling;
  config.data_channel_signaling = data_channel_signaling;
  config.data_channel_signaling_timeout = data_channel_signaling_timeout;
  config.enable_ignore_disconnect_websocket =
      enable_ignore_disconnect_websocket;
  config.ignore_disconnect_websocket = ignore_disconnect_websocket;
  config.disconnect_wait_timeout = disconnect_wait_timeout;
  if (!sora->Connect(config)) {
    return -1;
  }
  return 0;
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
  sora->GetStats([f, userdata](std::string json) {
    f(json.c_str(), json.size(), userdata);
  });
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
