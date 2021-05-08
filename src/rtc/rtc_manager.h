#ifndef SORA_RTC_MANAGER_H_
#define SORA_RTC_MANAGER_H_

// WebRTC
#include <api/peer_connection_interface.h>
#include <api/video/video_frame.h>
#include <pc/video_track_source.h>

#include "rtc_connection.h"
#include "rtc_data_manager.h"
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

  bool insecure = false;
  bool simulcast = false;
};

class RTCManager {
 public:
  static std::unique_ptr<RTCManager> Create(
      RTCManagerConfig config,
      rtc::scoped_refptr<rtc::AdaptedVideoTrackSource> video_track_source,
      VideoTrackReceiver* receiver,
      rtc::scoped_refptr<webrtc::AudioDeviceModule> adm,
      std::unique_ptr<webrtc::TaskQueueFactory> task_queue_factory,
      std::unique_ptr<rtc::Thread> signaling_thread,
      std::unique_ptr<rtc::Thread> worker_thread);

 private:
  RTCManager();
  bool Init(RTCManagerConfig config,
            rtc::scoped_refptr<rtc::AdaptedVideoTrackSource> video_track_source,
            VideoTrackReceiver* receiver,
            rtc::scoped_refptr<webrtc::AudioDeviceModule> adm,
            std::unique_ptr<webrtc::TaskQueueFactory> task_queue_factory,
            std::unique_ptr<rtc::Thread> signaling_thread,
            std::unique_ptr<rtc::Thread> worker_thread);

  // RTCDataManager が生きてる場合だけ OnDataChannel を呼ぶためのプロキシ
  struct RTCDataManagerProxy : RTCDataManager {
    void SetDataManager(std::shared_ptr<RTCDataManager> data_manager) {
      data_manager_ = data_manager;
    }
    void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface>
                           data_channel) override {
      auto data_manager = data_manager_.lock();
      if (data_manager) {
        data_manager->OnDataChannel(data_channel);
      }
    };
    std::weak_ptr<RTCDataManager> data_manager_;
  };

 public:
  ~RTCManager();
  void SetDataManager(std::shared_ptr<RTCDataManager> data_manager);
  std::shared_ptr<RTCConnection> CreateConnection(
      webrtc::PeerConnectionInterface::RTCConfiguration rtc_config,
      RTCMessageSender* sender);
  void InitTracks(RTCConnection* conn);

 private:
  static bool InitADM(rtc::scoped_refptr<webrtc::AudioDeviceModule> adm,
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
  RTCDataManagerProxy data_manager_proxy_;
};

}  // namespace sora

#endif  // SORA_RTC_MANAGER_H_
