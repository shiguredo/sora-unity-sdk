#include "unity.h"
#include "sora.h"

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

void sora_dispatch_events(void* p) {
  auto sora = (sora::Sora*)p;
  sora->DispatchEvents();
}

int sora_connect(void* p,
                 const char* signaling_url,
                 const char* channel_id,
                 bool downstream,
                 bool multistream,
                 int capturer_type,
                 void* unity_camera_texture,
                 int video_width,
                 int video_height) {
  auto sora = (sora::Sora*)p;
  if (!sora->Connect(signaling_url, channel_id, downstream, multistream,
                     capturer_type, unity_camera_texture, video_width,
                     video_height)) {
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

void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
UnityPluginLoad(IUnityInterfaces* ifs) {
  sora::UnityContext::Instance().Init(ifs);
}
void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload() {
  sora::UnityContext::Instance().Shutdown();
}
}
