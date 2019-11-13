#include "device_list.h"

// webrtc
#include "api/task_queue/default_task_queue_factory.h"
#include "modules/audio_device/include/audio_device.h"
#include "modules/audio_device/include/audio_device_factory.h"
#include "modules/video_capture/video_capture.h"
#include "modules/video_capture/video_capture_factory.h"
#include "rtc_base/logging.h"

namespace sora {

bool DeviceList::EnumVideoCapturer(
    std::function<void(std::string, std::string)> f) {
  std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
      webrtc::VideoCaptureFactory::CreateDeviceInfo());
  if (!info) {
    RTC_LOG(LS_WARNING) << "Failed to CreateDeviceInfo";
    return false;
  }

  int num_devices = info->NumberOfDevices();
  for (int i = 0; i < num_devices; i++) {
    char device_name[256];
    char unique_name[256];
    if (info->GetDeviceName(i, device_name, sizeof(device_name), unique_name,
                            sizeof(unique_name)) != 0) {
      RTC_LOG(LS_WARNING) << "Failed to GetDeviceName: index=" << i;
      continue;
    }

    RTC_LOG(LS_INFO) << "EnumVideoCapturer: device_name=" << device_name
                     << " unique_name=" << unique_name;
    f(device_name, unique_name);
  }
  return true;
}

bool DeviceList::EnumAudioRecording(
    std::function<void(std::string, std::string)> f) {
  auto task_queue_factory = webrtc::CreateDefaultTaskQueueFactory();
#ifdef _WIN32
  auto adm =
      webrtc::CreateWindowsCoreAudioAudioDeviceModule(task_queue_factory.get());
#else
  auto adm = webrtc::AudioDeviceModule::Create(
      webrtc::AudioDeviceModule::kPlatformDefaultAudio,
      task_queue_factory.get());
#endif
  int devices = adm->RecordingDevices();
  for (int i = 0; i < devices; i++) {
    char name[webrtc::kAdmMaxDeviceNameSize];
    char guid[webrtc::kAdmMaxGuidSize];
    if (adm->SetRecordingDevice(i) != 0) {
      RTC_LOG(LS_WARNING) << "Failed to SetRecordingDevice: index=" << i;
      continue;
    }
    bool available = false;
    if (adm->RecordingIsAvailable(&available) != 0) {
      RTC_LOG(LS_WARNING) << "Failed to RecordingIsAvailable: index=" << i;
      continue;
    }

    if (!available) {
      continue;
    }
    if (adm->RecordingDeviceName(i, name, guid) != 0) {
      RTC_LOG(LS_WARNING) << "Failed to RecordingDeviceName: index=" << i;
      continue;
    }

    RTC_LOG(LS_INFO) << "EnumAudioRecording: device_name=" << name
                     << " unique_name=" << guid;
    f(name, guid);
  }
  return true;
}

bool DeviceList::EnumAudioPlayout(
    std::function<void(std::string, std::string)> f) {
  auto task_queue_factory = webrtc::CreateDefaultTaskQueueFactory();
#ifdef _WIN32
  auto adm =
      webrtc::CreateWindowsCoreAudioAudioDeviceModule(task_queue_factory.get());
#else
  auto adm = webrtc::AudioDeviceModule::Create(
      webrtc::AudioDeviceModule::kPlatformDefaultAudio,
      task_queue_factory.get());
#endif
  int devices = adm->PlayoutDevices();
  for (int i = 0; i < devices; i++) {
    char name[webrtc::kAdmMaxDeviceNameSize];
    char guid[webrtc::kAdmMaxGuidSize];
    if (adm->SetPlayoutDevice(i) != 0) {
      RTC_LOG(LS_WARNING) << "Failed to SetPlayoutDevice: index=" << i;
      continue;
    }
    bool available = false;
    if (adm->PlayoutIsAvailable(&available) != 0) {
      RTC_LOG(LS_WARNING) << "Failed to PlayoutIsAvailable: index=" << i;
      continue;
    }

    if (!available) {
      continue;
    }
    if (adm->PlayoutDeviceName(i, name, guid) != 0) {
      RTC_LOG(LS_WARNING) << "Failed to PlayoutDeviceName: index=" << i;
      continue;
    }

    RTC_LOG(LS_INFO) << "EnumAudioPlayout: device_name=" << name
                     << " unique_name=" << guid;
    f(name, guid);
  }
  return true;
}

}  // namespace sora