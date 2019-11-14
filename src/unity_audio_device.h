#ifndef SORA_UNITY_AUDIO_DEVICE_H_INCLUDED
#define SORA_UNITY_AUDIO_DEVICE_H_INCLUDED

#include <stddef.h>
#include <atomic>
#include <memory>
#include <vector>

// webrtc
#include "modules/audio_device/audio_device_buffer.h"
#include "modules/audio_device/include/audio_device.h"
#include "rtc_base/ref_counted_object.h"

namespace sora {

class UnityAudioDevice : public webrtc::AudioDeviceModule {
 public:
  UnityAudioDevice(rtc::scoped_refptr<webrtc::AudioDeviceModule> adm,
                   webrtc::TaskQueueFactory* task_queue_factory)
      : adm_(adm), task_queue_factory_(task_queue_factory) {}

  static rtc::scoped_refptr<UnityAudioDevice> Create(
      rtc::scoped_refptr<webrtc::AudioDeviceModule> adm,
      webrtc::TaskQueueFactory* task_queue_factory) {
    return new rtc::RefCountedObject<UnityAudioDevice>(adm, task_queue_factory);
  }

  void ProcessAudioData(const float* data, int32_t size) {
    if (started_ && is_recording_) {
      for (int i = 0; i < size; i++) {
#pragma warning(suppress : 4244)
        converted_audio_data_.push_back(data[i] >= 0 ? data[i] * SHRT_MAX
                                                     : data[i] * -SHRT_MIN);
      }
      //opus supports up to 48khz sample rate, enforce 48khz here for quality
      int chunkSize = 48000 * 2 / 100;
      while (converted_audio_data_.size() > chunkSize) {
        device_buffer_->SetRecordedBuffer(converted_audio_data_.data(),
                                          chunkSize / 2);
        device_buffer_->DeliverRecordedData();
        converted_audio_data_.erase(converted_audio_data_.begin(),
                                    converted_audio_data_.begin() + chunkSize);
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
    device_buffer_->RegisterAudioCallback(audioCallback);
    adm_->RegisterAudioCallback(audioCallback);
    return 0;
  }

  // Main initialization and termination
  virtual int32_t Init() override {
    device_buffer_ =
        std::make_unique<webrtc::AudioDeviceBuffer>(task_queue_factory_);
    started_ = true;
    return adm_->Init();
  }
  virtual int32_t Terminate() override {
    device_buffer_.reset();
    started_ = false;
    is_recording_ = false;
    return adm_->Terminate();
  }
  virtual bool Initialized() const override {
    return started_ && adm_->Initialized();
  }

  // Device enumeration
  virtual int16_t PlayoutDevices() override { return adm_->PlayoutDevices(); }
  virtual int16_t RecordingDevices() override { return 0; }
  virtual int32_t PlayoutDeviceName(
      uint16_t index,
      char name[webrtc::kAdmMaxDeviceNameSize],
      char guid[webrtc::kAdmMaxGuidSize]) override {
    return adm_->PlayoutDeviceName(index, name, guid);
  }
  virtual int32_t RecordingDeviceName(
      uint16_t index,
      char name[webrtc::kAdmMaxDeviceNameSize],
      char guid[webrtc::kAdmMaxGuidSize]) override {
    return 0;
  }

  // Device selection
  virtual int32_t SetPlayoutDevice(uint16_t index) override {
    return adm_->SetPlayoutDevice(index);
  }
  virtual int32_t SetPlayoutDevice(WindowsDeviceType device) override {
    return adm_->SetPlayoutDevice(device);
  }
  virtual int32_t SetRecordingDevice(uint16_t index) override { return 0; }
  virtual int32_t SetRecordingDevice(WindowsDeviceType device) override {
    return 0;
  }

  // Audio transport initialization
  virtual int32_t PlayoutIsAvailable(bool* available) override {
    return adm_->PlayoutIsAvailable(available);
  }
  virtual int32_t InitPlayout() override { return adm_->InitPlayout(); }
  virtual bool PlayoutIsInitialized() const override {
    return adm_->PlayoutIsInitialized();
  }
  virtual int32_t RecordingIsAvailable(bool* available) override { return 0; }
  virtual int32_t InitRecording() override {
    is_recording_ = true;
    device_buffer_->SetRecordingSampleRate(48000);
    device_buffer_->SetRecordingChannels(2);
    return 0;
  }
  virtual bool RecordingIsInitialized() const override { return is_recording_; }

  // Audio transport control
  virtual int32_t StartPlayout() override { return adm_->StartPlayout(); }
  virtual int32_t StopPlayout() override { return adm_->StopPlayout(); }
  virtual bool Playing() const override { return adm_->Playing(); }
  virtual int32_t StartRecording() override { return 0; }
  virtual int32_t StopRecording() override { return 0; }
  virtual bool Recording() const override { return is_recording_; }

  // Audio mixer initialization
  virtual int32_t InitSpeaker() override { return adm_->InitSpeaker(); }
  virtual bool SpeakerIsInitialized() const override {
    return adm_->SpeakerIsInitialized();
  }
  virtual int32_t InitMicrophone() override { return 0; }
  virtual bool MicrophoneIsInitialized() const override { return false; }

  // Speaker volume controls
  virtual int32_t SpeakerVolumeIsAvailable(bool* available) override {
    return adm_->SpeakerVolumeIsAvailable(available);
  }
  virtual int32_t SetSpeakerVolume(uint32_t volume) override {
    return adm_->SetSpeakerVolume(volume);
  }
  virtual int32_t SpeakerVolume(uint32_t* volume) const override {
    return adm_->SpeakerVolume(volume);
  }
  virtual int32_t MaxSpeakerVolume(uint32_t* maxVolume) const override {
    return adm_->MaxSpeakerVolume(maxVolume);
  }
  virtual int32_t MinSpeakerVolume(uint32_t* minVolume) const override {
    return adm_->MinSpeakerVolume(minVolume);
  }

  // Microphone volume controls
  virtual int32_t MicrophoneVolumeIsAvailable(bool* available) override {
    return 0;
  }
  virtual int32_t SetMicrophoneVolume(uint32_t volume) override { return 0; }
  virtual int32_t MicrophoneVolume(uint32_t* volume) const override {
    return 0;
  }
  virtual int32_t MaxMicrophoneVolume(uint32_t* maxVolume) const override {
    return 0;
  }
  virtual int32_t MinMicrophoneVolume(uint32_t* minVolume) const override {
    return 0;
  }

  // Speaker mute control
  virtual int32_t SpeakerMuteIsAvailable(bool* available) override {
    return adm_->SpeakerMuteIsAvailable(available);
  }
  virtual int32_t SetSpeakerMute(bool enable) override {
    return adm_->SetSpeakerMute(enable);
  }
  virtual int32_t SpeakerMute(bool* enabled) const override {
    return adm_->SpeakerMute(enabled);
  }

  // Microphone mute control
  virtual int32_t MicrophoneMuteIsAvailable(bool* available) override {
    return 0;
  }
  virtual int32_t SetMicrophoneMute(bool enable) override { return 0; }
  virtual int32_t MicrophoneMute(bool* enabled) const override { return 0; }

  // Stereo support
  virtual int32_t StereoPlayoutIsAvailable(bool* available) const override {
    return adm_->StereoPlayoutIsAvailable(available);
  }
  virtual int32_t SetStereoPlayout(bool enable) override {
    return adm_->SetStereoPlayout(enable);
  }
  virtual int32_t StereoPlayout(bool* enabled) const override {
    return adm_->StereoPlayoutIsAvailable(enabled);
  }
  virtual int32_t StereoRecordingIsAvailable(bool* available) const override {
    *available = true;
    return 0;
  }
  virtual int32_t SetStereoRecording(bool enable) override { return 0; }
  virtual int32_t StereoRecording(bool* enabled) const override {
    *enabled = true;
    return 0;
  }

  // Playout delay
  virtual int32_t PlayoutDelay(uint16_t* delayMS) const override {
    return adm_->PlayoutDelay(delayMS);
  }

  // Only supported on Android.
  virtual bool BuiltInAECIsAvailable() const override { return false; }
  virtual bool BuiltInAGCIsAvailable() const override { return false; }
  virtual bool BuiltInNSIsAvailable() const override { return false; }

  // Enables the built-in audio effects. Only supported on Android.
  virtual int32_t EnableBuiltInAEC(bool enable) override { return 0; }
  virtual int32_t EnableBuiltInAGC(bool enable) override { return 0; }
  virtual int32_t EnableBuiltInNS(bool enable) override { return 0; }

 private:
  rtc::scoped_refptr<webrtc::AudioDeviceModule> adm_;
  webrtc::TaskQueueFactory* task_queue_factory_;
  std::unique_ptr<webrtc::AudioDeviceBuffer> device_buffer_;
  std::atomic_bool started_;
  std::atomic_bool is_recording_;
  std::vector<int16_t> converted_audio_data_;
};

}  // namespace sora

#endif  // SORA_UNITY_AUDIO_DEVICE_H_INCLUDED
