#ifndef SORA_UNITY_SDK_UNITY_H_
#define SORA_UNITY_SDK_UNITY_H_

#include <stdint.h>

#include "unity/IUnityInterface.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int ptrid_t;
typedef int32_t unity_bool_t;

typedef void (*track_cb_t)(ptrid_t video_sink_id,
                           const char* connection_id,
                           void* userdata);
typedef void (*media_stream_track_cb_t)(void* transceiver,
                                        void* track,
                                        const char* connection_id,
                                        void* userdata);
typedef void (*remove_media_stream_track_cb_t)(void* receiver,
                                               void* track,
                                               const char* connection_id,
                                               void* userdata);
typedef void (*set_offer_cb_t)(const char* json, void* userdata);
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
typedef void (*data_channel_cb_t)(const char* reason, void* userdata);
typedef void (*capturer_frame_cb_t)(const char* data, void* userdata);

UNITY_INTERFACE_EXPORT void* sora_create();
UNITY_INTERFACE_EXPORT void sora_set_on_add_track(void* p,
                                                  track_cb_t on_add_track,
                                                  void* userdata);
UNITY_INTERFACE_EXPORT void sora_set_on_remove_track(void* p,
                                                     track_cb_t on_remove_track,
                                                     void* userdata);
UNITY_INTERFACE_EXPORT void sora_set_on_media_stream_track(
    void* p,
    media_stream_track_cb_t on_media_stream_track,
    void* userdata);
UNITY_INTERFACE_EXPORT void sora_set_on_remove_media_stream_track(
    void* p,
    remove_media_stream_track_cb_t on_remove_media_stream_track,
    void* userdata);
UNITY_INTERFACE_EXPORT void sora_set_on_set_offer(void* p,
                                                  set_offer_cb_t on_set_offer,
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
UNITY_INTERFACE_EXPORT void sora_set_on_data_channel(
    void* p,
    data_channel_cb_t on_data_channel,
    void* userdata);
UNITY_INTERFACE_EXPORT void sora_set_on_capturer_frame(void* p,
                                                       capturer_frame_cb_t f,
                                                       void* userdata);
UNITY_INTERFACE_EXPORT void sora_dispatch_events(void* p);
UNITY_INTERFACE_EXPORT void sora_connect(void* p, const char* config);
UNITY_INTERFACE_EXPORT void sora_disconnect(void* p);
UNITY_INTERFACE_EXPORT void sora_switch_camera(void* p, const char* config);
UNITY_INTERFACE_EXPORT void* sora_get_texture_update_callback();
UNITY_INTERFACE_EXPORT void sora_destroy(void* sora);

UNITY_INTERFACE_EXPORT void* sora_get_render_callback();
UNITY_INTERFACE_EXPORT int sora_get_render_callback_event_id(void* p);

UNITY_INTERFACE_EXPORT void* sora_get_video_track_from_video_sink_id(
    void* p,
    ptrid_t video_sink_id);
UNITY_INTERFACE_EXPORT ptrid_t
sora_get_video_sink_id_from_video_track(void* p, void* video_track);

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
UNITY_INTERFACE_EXPORT void sora_setenv(const char* name, const char* value);

UNITY_INTERFACE_EXPORT void* sora_get_audio_track(void* p);
UNITY_INTERFACE_EXPORT void* sora_get_video_track(void* p);
UNITY_INTERFACE_EXPORT unity_bool_t sora_get_audio_enabled(void* p);
UNITY_INTERFACE_EXPORT void sora_set_audio_enabled(void* p,
                                                   unity_bool_t enabled);
UNITY_INTERFACE_EXPORT unity_bool_t sora_get_video_enabled(void* p);
UNITY_INTERFACE_EXPORT void sora_set_video_enabled(void* p,
                                                   unity_bool_t enabled);

UNITY_INTERFACE_EXPORT int sora_get_selected_signaling_url_size(void* p);
UNITY_INTERFACE_EXPORT int sora_get_connected_signaling_url_size(void* p);
UNITY_INTERFACE_EXPORT void sora_get_selected_signaling_url(void* p,
                                                            void* buf,
                                                            int size);
UNITY_INTERFACE_EXPORT void sora_get_connected_signaling_url(void* p,
                                                             void* buf,
                                                             int size);
UNITY_INTERFACE_EXPORT int sora_get_video_codec_capability_size(
    const char* config);
UNITY_INTERFACE_EXPORT void sora_get_video_codec_capability(const char* config,
                                                            void* buf,
                                                            int size);
UNITY_INTERFACE_EXPORT unity_bool_t
sora_video_codec_preference_has_implementation(const char* self,
                                               const char* implementation);
UNITY_INTERFACE_EXPORT int sora_video_codec_preference_merge_size(
    const char* self,
    const char* preference);
UNITY_INTERFACE_EXPORT void sora_video_codec_preference_merge(
    const char* self,
    const char* preference,
    void* buf,
    int size);
UNITY_INTERFACE_EXPORT int
sora_create_video_codec_preference_from_implementation_size(
    const char* capability,
    const char* implementation);
UNITY_INTERFACE_EXPORT void
sora_create_video_codec_preference_from_implementation(
    const char* capability,
    const char* implementation,
    void* buf,
    int size);
UNITY_INTERFACE_EXPORT int sora_video_codec_capability_to_json_size(
    const char* self);
UNITY_INTERFACE_EXPORT void
sora_video_codec_capability_to_json(const char* self, void* buf, int size);
UNITY_INTERFACE_EXPORT int sora_video_codec_preference_to_json_size(
    const char* self);
UNITY_INTERFACE_EXPORT void
sora_video_codec_preference_to_json(const char* self, void* buf, int size);

// MediaStreamTrack
UNITY_INTERFACE_EXPORT int sora_media_stream_track_get_kind_size(void* p);
UNITY_INTERFACE_EXPORT void sora_media_stream_track_get_kind(void* p,
                                                             void* buf,
                                                             int size);
UNITY_INTERFACE_EXPORT int sora_media_stream_track_get_id_size(void* p);
UNITY_INTERFACE_EXPORT void sora_media_stream_track_get_id(void* p,
                                                           void* buf,
                                                           int size);

// AudioTrackSink
typedef void (*audio_track_sink_on_data_cb_t)(
    const short* data,
    int bits_per_sample,
    int sample_rate,
    int number_of_channels,
    int number_of_frames,
    const long* absolute_capture_timestamp_ms,
    void* userdata);
typedef int (*audio_track_sink_num_preferred_channels_cb_t)(void* userdata);
UNITY_INTERFACE_EXPORT void* sora_audio_track_sink_create(
    audio_track_sink_on_data_cb_t on_data,
    audio_track_sink_num_preferred_channels_cb_t num_preferred_channels,
    void* userdata);
UNITY_INTERFACE_EXPORT void sora_audio_track_sink_destroy(void* p);

// AudioTrack
UNITY_INTERFACE_EXPORT void sora_audio_track_add_sink(void* track, void* sink);
UNITY_INTERFACE_EXPORT void sora_audio_track_remove_sink(void* track,
                                                         void* sink);

// RtpTransceiver
UNITY_INTERFACE_EXPORT void* sora_rtp_transceiver_get_receiver(void* p);

// RtpReceiver
UNITY_INTERFACE_EXPORT int sora_rtp_receiver_get_info_size(void* p);
UNITY_INTERFACE_EXPORT void sora_rtp_receiver_get_info(void* p,
                                                       void* buf,
                                                       int size);

typedef void (*change_route_cb_t)(void* userdata);
UNITY_INTERFACE_EXPORT void* sora_audio_output_helper_create(
    change_route_cb_t cb,
    void* userdata);
UNITY_INTERFACE_EXPORT void sora_audio_output_helper_destroy(void* p);
UNITY_INTERFACE_EXPORT unity_bool_t
sora_audio_output_helper_is_handsfree(void* p);
UNITY_INTERFACE_EXPORT void sora_audio_output_helper_set_handsfree(
    void* p,
    unity_bool_t enabled);

#ifdef __cplusplus
}
#endif

#endif
