#ifndef SORA_DEVICE_LIST_H_
#define SORA_DEVICE_LIST_H_

#include <functional>
#include <string>

namespace sora {

class DeviceList {
 public:
  static bool EnumVideoCapturer(
      std::function<void(std::string, std::string)> f);
  static bool EnumAudioRecording(
      std::function<void(std::string, std::string)> f);
  static bool EnumAudioPlayout(std::function<void(std::string, std::string)> f);
};

}  // namespace sora

#endif  // SORA_DEVICE_LIST_H_
