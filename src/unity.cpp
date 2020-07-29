#include "unity.h"
#include "rtc/device_list.h"
#include "sora.h"

#if defined(SORA_UNITY_SDK_WINDOWS)
#include "hwenc_nvcodec/nvcodec_h264_encoder.h"
#include "hwenc_nvcodec/nvcodec_video_decoder.h"
#endif

extern "C" {

void* sora_create() {
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

void sora_dispatch_events(void* p) {
  auto sora = (sora::Sora*)p;
  sora->DispatchEvents();
}

int sora_connect(void* p,
                 const char* unity_version,
                 const char* signaling_url,
                 const char* channel_id,
                 const char* metadata,
                 const char* role,
                 bool multistream,
                 int capturer_type,
                 void* unity_camera_texture,
                 const char* video_capturer_device,
                 int video_width,
                 int video_height,
                 const char* video_codec,
                 int video_bitrate,
                 bool unity_audio_input,
                 bool unity_audio_output,
                 const char* audio_recording_device,
                 const char* audio_playout_device,
                 const char* audio_codec,
                 int audio_bitrate) {
  auto sora = (sora::Sora*)p;
  if (!sora->Connect(unity_version, signaling_url, channel_id, metadata, role,
                     multistream, capturer_type, unity_camera_texture,
                     video_capturer_device, video_width, video_height,
                     video_codec, video_bitrate, unity_audio_input,
                     unity_audio_output, audio_recording_device,
                     audio_playout_device, audio_codec, audio_bitrate)) {
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

bool sora_device_enum_video_capturer(device_enum_cb_t f, void* userdata) {
  return sora::DeviceList::EnumVideoCapturer(
      [f, userdata](std::string device_name, std::string unique_name) {
        f(device_name.c_str(), unique_name.c_str(), userdata);
      });
}
bool sora_device_enum_audio_recording(device_enum_cb_t f, void* userdata) {
  return sora::DeviceList::EnumAudioRecording(
      [f, userdata](std::string device_name, std::string unique_name) {
        f(device_name.c_str(), unique_name.c_str(), userdata);
      });
}
bool sora_device_enum_audio_playout(device_enum_cb_t f, void* userdata) {
  return sora::DeviceList::EnumAudioPlayout(
      [f, userdata](std::string device_name, std::string unique_name) {
        f(device_name.c_str(), unique_name.c_str(), userdata);
      });
}

bool sora_is_h264_supported() {
#if defined(SORA_UNITY_SDK_WINDOWS)
  return NvCodecH264Encoder::IsSupported() && NvCodecVideoDecoder::IsSupported(cudaVideoCodec_H264);
#elif defined(SORA_UNITY_SDK_MACOS)
  // macOS は VideoToolbox が使えるので常に true
  return true;
#elif defined(SORA_UNITY_SDK_ANDROID)
  // Android は多分大体動くので true
  return true;
#endif
}

void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
UnityPluginLoad(IUnityInterfaces* ifs) {
  sora::UnityContext::Instance().Init(ifs);
}
void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload() {
  sora::UnityContext::Instance().Shutdown();
}
}
