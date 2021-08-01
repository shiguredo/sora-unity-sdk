#ifndef SORA_UNITY_H_
#define SORA_UNITY_H_

#include <stdint.h>

#include "unity/IUnityInterface.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int ptrid_t;
typedef int32_t unity_bool_t;

typedef void (*track_cb_t)(ptrid_t track_id, void* userdata);
typedef void (*notify_cb_t)(const char* json, void* userdata);
typedef void (*push_cb_t)(const char* json, void* userdata);
typedef void (*stats_cb_t)(const char* json, void* userdata);
typedef void (*message_cb_t)(const char* label,
                             const void* buf,
                             int size,
                             void* userdata);
typedef void (*disconnect_cb_t)(int error_code,
                                const char* reason,
                                void* userdata);

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
UNITY_INTERFACE_EXPORT void sora_set_on_push(void* p,
                                             push_cb_t on_push,
                                             void* userdata);
UNITY_INTERFACE_EXPORT void sora_set_on_message(void* p,
                                                message_cb_t on_message,
                                                void* userdata);
UNITY_INTERFACE_EXPORT void
sora_set_on_disconnect(void* p, disconnect_cb_t on_disconnect, void* userdata);
UNITY_INTERFACE_EXPORT void sora_dispatch_events(void* p);
UNITY_INTERFACE_EXPORT void sora_connect(void* p, const char* config);
UNITY_INTERFACE_EXPORT void sora_disconnect(void* p);
UNITY_INTERFACE_EXPORT void* sora_get_texture_update_callback();
UNITY_INTERFACE_EXPORT void sora_destroy(void* sora);

UNITY_INTERFACE_EXPORT void* sora_get_render_callback();
UNITY_INTERFACE_EXPORT int sora_get_render_callback_event_id(void* p);

UNITY_INTERFACE_EXPORT void sora_process_audio(void* p,
                                               const void* buf,
                                               int offset,
                                               int samples);
typedef void (*handle_audio_cb_t)(const int16_t* buf,
                                  int samples,
                                  int channels,
                                  void* userdata);
UNITY_INTERFACE_EXPORT void sora_set_on_handle_audio(void* p,
                                                     handle_audio_cb_t f,
                                                     void* userdata);

UNITY_INTERFACE_EXPORT void sora_get_stats(void* p,
                                           stats_cb_t f,
                                           void* userdata);

UNITY_INTERFACE_EXPORT void sora_send_message(void* p,
                                              const char* label,
                                              void* buf,
                                              int size);

typedef void (*device_enum_cb_t)(const char* device_name,
                                 const char* unique_name,
                                 void* userdata);
UNITY_INTERFACE_EXPORT unity_bool_t
sora_device_enum_video_capturer(device_enum_cb_t f, void* userdata);
UNITY_INTERFACE_EXPORT unity_bool_t
sora_device_enum_audio_recording(device_enum_cb_t f, void* userdata);
UNITY_INTERFACE_EXPORT unity_bool_t
sora_device_enum_audio_playout(device_enum_cb_t f, void* userdata);
UNITY_INTERFACE_EXPORT unity_bool_t sora_is_h264_supported();

#ifdef __cplusplus
}
#endif

#endif  // SORA_UNITY_H_
