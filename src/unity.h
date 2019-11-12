#ifndef SORA_UNITY_H_
#define SORA_UNITY_H_

#include "unity/IUnityInterface.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int ptrid_t;

typedef void (*track_cb_t)(ptrid_t track_id, void* userdata);
typedef void (*notify_cb_t)(const char* json, int size, void* userdata);

UNITY_INTERFACE_EXPORT void* sora_create();
UNITY_INTERFACE_EXPORT void sora_set_on_add_track(void* p,
                                                  track_cb_t on_add_track,
                                                  void* userdata);
UNITY_INTERFACE_EXPORT void sora_set_on_remove_track(void* p,
                                                     track_cb_t on_remove_track,
                                                     void* userdata);
UNITY_INTERFACE_EXPORT void sora_set_on_notify(void* p,
                                               notify_cb_t on_notify,
                                               void* userdata);
UNITY_INTERFACE_EXPORT void sora_dispatch_events(void* p);
UNITY_INTERFACE_EXPORT int sora_connect(void* p,
                                        const char* signaling_url,
                                        const char* channel_id,
                                        const char* metadata,
                                        bool downstream,
                                        bool multistream,
                                        int capturer_type,
                                        void* unity_camera_texture,
                                        int video_width,
                                        int video_height,
                                        bool unity_audio_input);
UNITY_INTERFACE_EXPORT void* sora_get_texture_update_callback();
UNITY_INTERFACE_EXPORT void sora_destroy(void* sora);

UNITY_INTERFACE_EXPORT void* sora_get_render_callback();
UNITY_INTERFACE_EXPORT int sora_get_render_callback_event_id(void* p);

UNITY_INTERFACE_EXPORT void sora_process_audio(void* p,
                                               const void* buf,
                                               int offset,
                                               int samples);

#ifdef __cplusplus
}
#endif

#endif  // SORA_UNITY_H_