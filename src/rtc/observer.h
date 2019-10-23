#ifndef SORA_OBSERVER_H_
#define SORA_OBSERVER_H_

#include "api/peer_connection_interface.h"
#include "api/rtp_transceiver_interface.h"
#include "api/video/video_frame.h"
#include "api/video/video_sink_interface.h"

#include "rtc_message_sender.h"
#include "video_track_receiver.h"

namespace sora {

class PeerConnectionObserver : public webrtc::PeerConnectionObserver {
 public:
  PeerConnectionObserver(RTCMessageSender* sender, VideoTrackReceiver* receiver)
      : sender_(sender), receiver_(receiver) {}
  ~PeerConnectionObserver() { ClearAllRegisteredTracks(); }

 protected:
  void OnSignalingChange(
      webrtc::PeerConnectionInterface::SignalingState new_state) override {}
  void OnAddStream(
      rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override {}
  void OnRemoveStream(
      rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override {}
  void OnDataChannel(
      rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) override {}
  void OnRenegotiationNeeded() override {}
  void OnIceConnectionChange(
      webrtc::PeerConnectionInterface::IceConnectionState new_state) override;
  void OnIceGatheringChange(
      webrtc::PeerConnectionInterface::IceGatheringState new_state) override{};
  void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;
  void OnIceConnectionReceivingChange(bool receiving) override {}
  void OnTrack(
      rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) override;
  void OnRemoveTrack(
      rtc::scoped_refptr<webrtc::RtpReceiverInterface> transceiver) override;

 private:
  void ClearAllRegisteredTracks();

  RTCMessageSender* sender_;
  VideoTrackReceiver* receiver_;
  std::vector<webrtc::VideoTrackInterface*> video_tracks_;
};

class CreateSessionDescriptionObserver
    : public webrtc::CreateSessionDescriptionObserver {
 public:
  static CreateSessionDescriptionObserver* Create(
      RTCMessageSender* sender,
      webrtc::PeerConnectionInterface* connection) {
    return new rtc::RefCountedObject<CreateSessionDescriptionObserver>(
        sender, connection);
  }

 protected:
  CreateSessionDescriptionObserver(RTCMessageSender* sender,
                                   webrtc::PeerConnectionInterface* connection)
      : sender_(sender), _connection(connection){};
  void OnSuccess(webrtc::SessionDescriptionInterface* desc) override;
  void OnFailure(webrtc::RTCError error) override;

 private:
  RTCMessageSender* sender_;
  webrtc::PeerConnectionInterface* _connection;
};

class SetSessionDescriptionObserver
    : public webrtc::SetSessionDescriptionObserver {
 public:
  static SetSessionDescriptionObserver* Create(webrtc::SdpType type,
                                               RTCMessageSender* sender) {
    return new rtc::RefCountedObject<SetSessionDescriptionObserver>(type,
                                                                    sender);
  }

 protected:
  SetSessionDescriptionObserver(webrtc::SdpType type, RTCMessageSender* sender)
      : _type(type), sender_(sender){};
  void OnSuccess() override;
  void OnFailure(webrtc::RTCError error) override;

 private:
  RTCMessageSender* sender_;
  webrtc::SdpType _type;
};

}  // namespace sora

#endif
