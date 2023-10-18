#include "ios_audio_init.h"

#include <rtc_base/logging.h>

#import <sdk/objc/components/audio/RTCAudioSession.h>
#import <sdk/objc/components/audio/RTCAudioSessionConfiguration.h>

namespace sora_unity_sdk {

void IosAudioInit(std::function<void(std::string)> on_complete) {
  auto config = [RTCAudioSessionConfiguration webRTCConfiguration];
  config.category = AVAudioSessionCategoryPlayAndRecord;
  [[RTCAudioSession sharedInstance] initializeInput:^(NSError* error) {
    if (error != nil) {
      on_complete([error.localizedDescription UTF8String]);
    } else {
      on_complete("");
    }
  }];
}

void IosSyncAudioSession() {
  auto config = [RTCAudioSessionConfiguration webRTCConfiguration];
  auto session = [AVAudioSession sharedInstance];
  config.category = session.category;
  config.mode = session.mode;
}

}
