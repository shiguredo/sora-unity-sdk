#ifndef SORA_UNITY_SDK_MAC_HELPER_IOS_AUDIO_INIT_H_
#define SORA_UNITY_SDK_MAC_HELPER_IOS_AUDIO_INIT_H_

#include <functional>
#include <string>

namespace sora_unity_sdk {

void IosAudioInit(std::function<void(std::string)> on_complete);

}

#endif
