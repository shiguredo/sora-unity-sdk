#ifndef SORA_UNITY_H_
#define SORA_UNITY_H_

#include "unity/IUnityInterface.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int ptrid_t;

typedef void (*track_cb_t)(ptrid_t track_id, void* userdata);

UNITY_INTERFACE_EXPORT void* sora_create();
UNITY_INTERFACE_EXPORT void sora_set_on_add_track(void* p,
                                                  track_cb_t on_add_track,
                                                  void* userdata);
UNITY_INTERFACE_EXPORT void sora_set_on_remove_track(void* p,
                                                     track_cb_t on_remove_track,
                                                     void* userdata);
UNITY_INTERFACE_EXPORT void sora_dispatch_events(void* p);
UNITY_INTERFACE_EXPORT int sora_connect(void* p,
                                        const char* signaling_url,
                                        const char* channel_id,
                                        bool downstream,
                                        bool multistream);
UNITY_INTERFACE_EXPORT void* sora_get_texture_update_callback();
UNITY_INTERFACE_EXPORT void sora_destroy(void* sora);

#ifdef __cplusplus
}
#endif

#endif  // SORA_UNITY_H_