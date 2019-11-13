#ifndef SORA_RTC_MANAGER_H_
#define SORA_RTC_MANAGER_H_

#include "api/peer_connection_interface.h"
#include "api/video/video_frame.h"
#include "pc/video_track_source.h"

#include "rtc_connection.h"
#include "scalable_track_source.h"
#include "video_track_receiver.h"

namespace sora {

struct RTCManagerConfig {
  bool no_video = false;
  bool fixed_resolution = false;

  bool no_recording = false;
  bool no_playout = false;
  bool disable_echo_cancellation = false;
  bool disable_auto_gain_control = false;
  bool disable_noise_suppression = false;
  bool disable_highpass_filter = false;
  bool disable_typing_detection = false;

  std::string audio_recording_device;
  std::string audio_playout_device;

  // webrtc::DegradationPreference::MAINTAIN_RESOLUTION;
  // webrtc::DegradationPreference::MAINTAIN_FRAMERATE;
  webrtc::DegradationPreference priority =
      webrtc::DegradationPreference::BALANCED;
};

class RTCManager {
 public:
  static std::unique_ptr<RTCManager> Create(
      RTCManagerConfig config,
      rtc::scoped_refptr<ScalableVideoTrackSource> video_track_source,
      VideoTrackReceiver* receiver,
      rtc::scoped_refptr<webrtc::AudioDeviceModule> adm,
      std::unique_ptr<webrtc::TaskQueueFactory> task_queue_factory);

 private:
  RTCManager();
  bool Init(RTCManagerConfig config,
            rtc::scoped_refptr<ScalableVideoTrackSource> video_track_source,
            VideoTrackReceiver* receiver,
            rtc::scoped_refptr<webrtc::AudioDeviceModule> adm,
            std::unique_ptr<webrtc::TaskQueueFactory> task_queue_factory);

 public:
  ~RTCManager();
  std::shared_ptr<RTCConnection> createConnection(
      webrtc::PeerConnectionInterface::RTCConfiguration rtc_config,
      RTCMessageSender* sender);

 private:
  static bool RTCManager::InitADM(
      rtc::scoped_refptr<webrtc::AudioDeviceModule> adm,
      std::string audio_recording_device,
      std::string audio_playout_device);

 private:
  rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory_;
  rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track_;
  rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track_;
  VideoTrackReceiver* receiver_;
  std::unique_ptr<rtc::Thread> network_thread_;
  std::unique_ptr<rtc::Thread> worker_thread_;
  std::unique_ptr<rtc::Thread> signaling_thread_;
  RTCManagerConfig config_;
};

}  // namespace sora

#endif  // SORA_RTC_MANAGER_H_
