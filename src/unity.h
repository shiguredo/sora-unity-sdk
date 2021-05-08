#ifndef SORA_UNITY_H_
#define SORA_UNITY_H_

#include <stdint.h>

#include "unity/IUnityInterface.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int ptrid_t;

typedef void (*track_cb_t)(ptrid_t track_id, void* userdata);
typedef void (*notify_cb_t)(const char* json, int size, void* userdata);
typedef void (*push_cb_t)(const char* json, int size, void* userdata);
typedef void (*stats_cb_t)(const char* json, int size, void* userdata);

typedef int32_t unity_bool_t;

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
UNITY_INTERFACE_EXPORT void sora_dispatch_events(void* p);
UNITY_INTERFACE_EXPORT int sora_connect(void* p,
                                        const char* unity_version,
                                        const char* signaling_url,
                                        const char* channel_id,
                                        const char* metadata,
                                        const char* role,
                                        unity_bool_t multistream,
                                        unity_bool_t spotlight,
                                        int spotlight_number,
                                        unity_bool_t simulcast,
                                        int capturer_type,
                                        void* unity_camera_texture,
                                        const char* video_capturer_device,
                                        int video_width,
                                        int video_height,
                                        const char* video_codec,
                                        int video_bitrate,
                                        unity_bool_t unity_audio_input,
                                        unity_bool_t unity_audio_output,
                                        const char* audio_recording_device,
                                        const char* audio_playout_device,
                                        const char* audio_codec,
                                        int audio_bitrate);
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

UNITY_INTERFACE_EXPORT void sora_set_audio_enabled(void* p,
                                                   int status);

UNITY_INTERFACE_EXPORT void sora_set_video_enabled(void* p,
                                                   int status);

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
