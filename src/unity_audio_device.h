#ifndef SORA_UNITY_SDK_UNITY_AUDIO_DEVICE_H_INCLUDED
#define SORA_UNITY_SDK_UNITY_AUDIO_DEVICE_H_INCLUDED

#include <stddef.h>
#include <atomic>
#include <memory>
#include <vector>

// webrtc
#include "modules/audio_device/audio_device_buffer.h"
#include "modules/audio_device/include/audio_device.h"
#include "rtc_base/ref_counted_object.h"
#include "rtc_base/thread.h"

namespace sora_unity_sdk {

class UnityAudioDevice : public webrtc::AudioDeviceModule {
 public:
  UnityAudioDevice(
      rtc::scoped_refptr<webrtc::AudioDeviceModule> adm,
      bool adm_recording,
      bool adm_playout,
      std::function<void(const int16_t* p, int samples, int channels)>
          on_handle_audio,
      webrtc::TaskQueueFactory* task_queue_factory)
      : adm_(adm),
        adm_recording_(adm_recording),
        adm_playout_(adm_playout),
        on_handle_audio_(on_handle_audio),
        task_queue_factory_(task_queue_factory) {}

  ~UnityAudioDevice() override {
    RTC_LOG(LS_INFO) << "~UnityAudioDevice";
    Terminate();
  }

  static rtc::scoped_refptr<UnityAudioDevice> Create(
      rtc::scoped_refptr<webrtc::AudioDeviceModule> adm,
      bool adm_recording,
      bool adm_playout,
      std::function<void(const int16_t* p, int samples, int channels)>
          on_handle_audio,
      webrtc::TaskQueueFactory* task_queue_factory) {
    return rtc::make_ref_counted<UnityAudioDevice>(
        adm, adm_recording, adm_playout, on_handle_audio, task_queue_factory);
  }

  void ProcessAudioData(const float* data, int32_t size) {
    if (!adm_recording_ && initialized_ && is_recording_) {
      for (int i = 0; i < size; i++) {
#pragma warning(suppress : 4244)
        converted_audio_data_.push_back(data[i] >= 0 ? data[i] * SHRT_MAX
                                                     : data[i] * -SHRT_MIN);
      }
      //opus supports up to 48khz sample rate, enforce 48khz here for quality
      int chunk_size = 48000 * 2 / 100;
      while (converted_audio_data_.size() > chunk_size) {
        device_buffer_->SetRecordedBuffer(converted_audio_data_.data(),
                                          chunk_size / 2);
        device_buffer_->DeliverRecordedData();
        converted_audio_data_.erase(converted_audio_data_.begin(),
                                    converted_audio_data_.begin() + chunk_size);
      }
    }
  }

  //webrtc::AudioDeviceModule
  // Retrieve the currently utilized audio layer
  virtual int32_t ActiveAudioLayer(AudioLayer* audioLayer) const override {
    //*audioLayer = AudioDeviceModule::kPlatformDefaultAudio;
    adm_->ActiveAudioLayer(audioLayer);
    return 0;
  }
  // Full-duplex transportation of PCM audio
  virtual int32_t RegisterAudioCallback(
      webrtc::AudioTransport* audioCallback) override {
    RTC_LOG(LS_INFO) << "RegisterAudioCallback";
    if (!adm_recording_ || !adm_playout_) {
      device_buffer_->RegisterAudioCallback(audioCallback);
    }
    return adm_->RegisterAudioCallback(audioCallback);
  }

  // Main initialization and termination
  virtual int32_t Init() override {
    RTC_LOG(LS_INFO) << "Init";
    device_buffer_ =
        std::make_unique<webrtc::AudioDeviceBuffer>(task_queue_factory_);
    initialized_ = true;
    return adm_->Init();
  }
  virtual int32_t Terminate() override {
    RTC_LOG(LS_INFO) << "Terminate";

    DoStopPlayout();

    initialized_ = false;
    is_recording_ = false;
    is_playing_ = false;
    device_buffer_.reset();

    auto result = adm_->Terminate();

    RTC_LOG(LS_INFO) << "Terminate Completed";

    return result;
  }
  virtual bool Initialized() const override {
    return initialized_ && adm_->Initialized();
  }

  // Device enumeration
  virtual int16_t PlayoutDevices() override {
    RTC_LOG(LS_INFO) << "PlayoutDevices";
    return adm_playout_ ? adm_->PlayoutDevices() : 0;
  }
  virtual int16_t RecordingDevices() override {
    return adm_recording_ ? adm_->RecordingDevices() : 0;
  }
  virtual int32_t PlayoutDeviceName(
      uint16_t index,
      char name[webrtc::kAdmMaxDeviceNameSize],
      char guid[webrtc::kAdmMaxGuidSize]) override {
    return adm_playout_ ? adm_->PlayoutDeviceName(index, name, guid) : 0;
  }
  virtual int32_t RecordingDeviceName(
      uint16_t index,
      char name[webrtc::kAdmMaxDeviceNameSize],
      char guid[webrtc::kAdmMaxGuidSize]) override {
    return adm_recording_ ? adm_->RecordingDeviceName(index, name, guid) : 0;
  }

  // Device selection
  virtual int32_t SetPlayoutDevice(uint16_t index) override {
    RTC_LOG(LS_INFO) << "SetPlayoutDevice(" << index << ")";
    return adm_playout_ ? adm_->SetPlayoutDevice(index) : 0;
  }
  virtual int32_t SetPlayoutDevice(WindowsDeviceType device) override {
    return adm_playout_ ? adm_->SetPlayoutDevice(device) : 0;
  }
  virtual int32_t SetRecordingDevice(uint16_t index) override {
    RTC_LOG(LS_INFO) << "SetRecordingDevice(" << index << ")";
    return adm_recording_ ? adm_->SetRecordingDevice(index) : 0;
  }
  virtual int32_t SetRecordingDevice(WindowsDeviceType device) override {
    return adm_recording_ ? adm_->SetRecordingDevice(device) : 0;
  }

  void HandleAudioData() {
    int channels = stereo_playout_ ? 2 : 1;
    auto next_at = std::chrono::steady_clock::now();
    while (!handle_audio_thread_stopped_) {
      // 10 ミリ秒ごとにオーディオデータを取得する
      next_at += std::chrono::milliseconds(10);
      std::this_thread::sleep_until(next_at);

      int chunk_size = 48000 / 100;
      int samples = device_buffer_->RequestPlayoutData(chunk_size);

      //RTC_LOG(LS_INFO) << "handle audio data: chunk_size=" << chunk_size
      //                 << " samples=" << samples;

      std::unique_ptr<int16_t[]> audio_buffer(new int16_t[samples * channels]);
      device_buffer_->GetPlayoutData(audio_buffer.get());
      if (on_handle_audio_) {
        on_handle_audio_(audio_buffer.get(), samples, channels);
      }
    }
  }

  // Audio transport initialization
  virtual int32_t PlayoutIsAvailable(bool* available) override {
    RTC_LOG(LS_INFO) << "PlayoutIsAvailable";

    if (adm_playout_) {
      return adm_->PlayoutIsAvailable(available);
    } else {
      *available = true;
      return 0;
    }
  }
  virtual int32_t InitPlayout() override {
    RTC_LOG(LS_INFO) << "InitPlayout";

    if (adm_playout_) {
      return adm_->InitPlayout();
    } else {
      DoStopPlayout();

      is_playing_ = true;
      device_buffer_->SetPlayoutSampleRate(48000);
      device_buffer_->SetPlayoutChannels(stereo_playout_ ? 2 : 1);

      return 0;
    }
  }
  virtual bool PlayoutIsInitialized() const override {
    auto result =
        adm_playout_ ? adm_->PlayoutIsInitialized() : (bool)is_playing_;
    RTC_LOG(LS_INFO) << "PlayoutIsInitialized: result=" << result;
    return result;
  }
  virtual int32_t RecordingIsAvailable(bool* available) override {
    if (adm_recording_) {
      return adm_->RecordingIsAvailable(available);
    } else {
      *available = true;
      return 0;
    }
  }
  virtual int32_t InitRecording() override {
    if (adm_recording_) {
      return adm_->InitRecording();
    } else {
      is_recording_ = true;
      device_buffer_->SetRecordingSampleRate(48000);
      device_buffer_->SetRecordingChannels(2);
      return 0;
    }
  }
  virtual bool RecordingIsInitialized() const override {
    return adm_recording_ ? adm_->RecordingIsInitialized()
                          : (bool)is_recording_;
  }

  // Audio transport control
  virtual int32_t StartPlayout() override {
    RTC_LOG(LS_INFO) << "StartPlayout";
    if (adm_playout_) {
      return adm_->StartPlayout();
    } else {
      handle_audio_thread_.reset(new std::thread([this]() {
        RTC_LOG(LS_INFO) << "Sora Audio Playout Thread started";
        HandleAudioData();
        RTC_LOG(LS_INFO) << "Sora Audio Playout Thread finished";
      }));

      return 0;
    }
  }
  void DoStopPlayout() {
    if (handle_audio_thread_) {
      RTC_LOG(LS_INFO) << "Terminating Audio Thread";
      handle_audio_thread_stopped_ = true;
      handle_audio_thread_->join();
      handle_audio_thread_.reset();
      handle_audio_thread_stopped_ = false;
      RTC_LOG(LS_INFO) << "Terminated Audio Thread";
    }
  }
  virtual int32_t StopPlayout() override {
    RTC_LOG(LS_INFO) << "StopPlayout";
    if (adm_playout_) {
      return adm_->StopPlayout();
    } else {
      DoStopPlayout();
      is_playing_ = false;
      return 0;
    }
  }
  virtual bool Playing() const override {
    return adm_playout_ ? adm_->Playing() : (bool)is_playing_;
  }
  virtual int32_t StartRecording() override {
    return adm_recording_ ? adm_->StartRecording() : 0;
  }
  virtual int32_t StopRecording() override {
    return adm_recording_ ? adm_->StopRecording() : 0;
  }
  virtual bool Recording() const override {
    return adm_recording_ ? adm_->Recording() : (bool)is_recording_;
  }

  // Audio mixer initialization
  virtual int32_t InitSpeaker() override {
    return adm_playout_ ? adm_->InitSpeaker() : 0;
  }
  virtual bool SpeakerIsInitialized() const override {
    return adm_playout_ ? adm_->SpeakerIsInitialized() : false;
  }
  virtual int32_t InitMicrophone() override {
    return adm_recording_ ? adm_->InitMicrophone() : 0;
  }
  virtual bool MicrophoneIsInitialized() const override {
    return adm_recording_ ? adm_->MicrophoneIsInitialized() : false;
  }

  // Speaker volume controls
  virtual int32_t SpeakerVolumeIsAvailable(bool* available) override {
    return adm_playout_ ? adm_->SpeakerVolumeIsAvailable(available) : 0;
  }
  virtual int32_t SetSpeakerVolume(uint32_t volume) override {
    return adm_playout_ ? adm_->SetSpeakerVolume(volume) : 0;
  }
  virtual int32_t SpeakerVolume(uint32_t* volume) const override {
    return adm_playout_ ? adm_->SpeakerVolume(volume) : 0;
  }
  virtual int32_t MaxSpeakerVolume(uint32_t* maxVolume) const override {
    return adm_playout_ ? adm_->MaxSpeakerVolume(maxVolume) : 0;
  }
  virtual int32_t MinSpeakerVolume(uint32_t* minVolume) const override {
    return adm_playout_ ? adm_->MinSpeakerVolume(minVolume) : 0;
  }

  // Microphone volume controls
  virtual int32_t MicrophoneVolumeIsAvailable(bool* available) override {
    return adm_recording_ ? adm_->MicrophoneVolumeIsAvailable(available) : 0;
  }
  virtual int32_t SetMicrophoneVolume(uint32_t volume) override {
    return adm_recording_ ? adm_->SetMicrophoneVolume(volume) : 0;
  }
  virtual int32_t MicrophoneVolume(uint32_t* volume) const override {
    return adm_recording_ ? adm_->MicrophoneVolume(volume) : 0;
  }
  virtual int32_t MaxMicrophoneVolume(uint32_t* maxVolume) const override {
    return adm_recording_ ? adm_->MaxMicrophoneVolume(maxVolume) : 0;
  }
  virtual int32_t MinMicrophoneVolume(uint32_t* minVolume) const override {
    return adm_recording_ ? adm_->MinMicrophoneVolume(minVolume) : 0;
  }

  // Speaker mute control
  virtual int32_t SpeakerMuteIsAvailable(bool* available) override {
    return adm_playout_ ? adm_->SpeakerMuteIsAvailable(available) : 0;
  }
  virtual int32_t SetSpeakerMute(bool enable) override {
    return adm_playout_ ? adm_->SetSpeakerMute(enable) : 0;
  }
  virtual int32_t SpeakerMute(bool* enabled) const override {
    return adm_playout_ ? adm_->SpeakerMute(enabled) : 0;
  }

  // Microphone mute control
  virtual int32_t MicrophoneMuteIsAvailable(bool* available) override {
    return adm_recording_ ? adm_->MicrophoneMuteIsAvailable(available) : 0;
  }
  virtual int32_t SetMicrophoneMute(bool enable) override {
    return adm_recording_ ? adm_->SetMicrophoneMute(enable) : 0;
  }
  virtual int32_t MicrophoneMute(bool* enabled) const override {
    return adm_recording_ ? adm_->MicrophoneMute(enabled) : 0;
  }

  // Stereo support
  virtual int32_t StereoPlayoutIsAvailable(bool* available) const override {
    if (adm_playout_) {
      return adm_->StereoPlayoutIsAvailable(available);
    } else {
      // 今はステレオには対応しない
      //*available = true;
      *available = false;
      return 0;
    }
  }
  virtual int32_t SetStereoPlayout(bool enable) override {
    if (adm_playout_) {
      return adm_->SetStereoPlayout(enable);
    } else {
      stereo_playout_ = enable;
      return 0;
    };
  }
  virtual int32_t StereoPlayout(bool* enabled) const override {
    if (adm_playout_) {
      return adm_->StereoPlayoutIsAvailable(enabled);
    } else {
      *enabled = stereo_playout_;
      return 0;
    }
  }
  virtual int32_t StereoRecordingIsAvailable(bool* available) const override {
    if (adm_recording_) {
      return adm_->StereoRecordingIsAvailable(available);
    } else {
      *available = true;
      return 0;
    }
  }
  virtual int32_t SetStereoRecording(bool enable) override {
    return adm_recording_ ? adm_->SetStereoRecording(enable) : 0;
  }
  virtual int32_t StereoRecording(bool* enabled) const override {
    if (adm_recording_) {
      return adm_->StereoRecording(enabled);
    } else {
      *enabled = true;
      return 0;
    }
  }

  // Playout delay
  virtual int32_t PlayoutDelay(uint16_t* delayMS) const override {
    return adm_playout_ ? adm_->PlayoutDelay(delayMS) : 0;
  }

  // Only supported on Android.
  virtual bool BuiltInAECIsAvailable() const override {
    return false;
  }
  virtual bool BuiltInAGCIsAvailable() const override {
    return false;
  }
  virtual bool BuiltInNSIsAvailable() const override {
    return false;
  }

  // Enables the built-in audio effects. Only supported on Android.
  virtual int32_t EnableBuiltInAEC(bool enable) override {
    return 0;
  }
  virtual int32_t EnableBuiltInAGC(bool enable) override {
    return 0;
  }
  virtual int32_t EnableBuiltInNS(bool enable) override {
    return 0;
  }

// Only supported on iOS.
#if defined(WEBRTC_IOS)
  virtual int GetPlayoutAudioParameters(
      webrtc::AudioParameters* params) const override {
    return -1;
  }
  virtual int GetRecordAudioParameters(
      webrtc::AudioParameters* params) const override {
    return -1;
  }
#endif  // WEBRTC_IOS

 private:
  rtc::scoped_refptr<webrtc::AudioDeviceModule> adm_;
  bool adm_recording_;
  bool adm_playout_;
  webrtc::TaskQueueFactory* task_queue_factory_;
  std::function<void(const int16_t* p, int samples, int channels)>
      on_handle_audio_;
  std::unique_ptr<std::thread> handle_audio_thread_;
  std::atomic_bool handle_audio_thread_stopped_ = {false};
  std::unique_ptr<webrtc::AudioDeviceBuffer> device_buffer_;
  std::atomic_bool initialized_ = {false};
  std::atomic_bool is_recording_ = {false};
  std::atomic_bool is_playing_ = {false};
  std::atomic_bool stereo_playout_ = {false};
  std::vector<int16_t> converted_audio_data_;
};

}  // namespace sora_unity_sdk

#endif
