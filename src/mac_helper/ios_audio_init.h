#ifndef IOS_AUDIO_INIT_H_
#define IOS_AUDIO_INIT_H_

#include <functional>
#include <string>

void IosAudioInit(std::function<void(std::string)> on_complete);

#endif
