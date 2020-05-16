#ifndef SORA_RTC_CONNECTION_H_
#define SORA_RTC_CONNECTION_H_

#include "api/peer_connection_interface.h"

#include "observer.h"

namespace sora {

class RTCConnection {
 public:
  RTCConnection(RTCMessageSender* sender,
                std::unique_ptr<PeerConnectionObserver> observer,
                rtc::scoped_refptr<webrtc::PeerConnectionInterface> connection)
      : sender_(sender),
        observer_(std::move(observer)),
        connection_(connection){};
  ~RTCConnection();
  void createOffer();
  void setOffer(const std::string sdp);
  void createAnswer();
  void setAnswer(const std::string sdp);
  void addIceCandidate(const std::string sdp_mid,
                       const int sdp_mlineindex,
                       const std::string sdp);
  bool setAudioEnabled(bool enabled);
  bool setVideoEnabled(bool enabled);
  bool isAudioEnabled();
  bool isVideoEnabled();

  void getStats(
      std::function<void(
          const rtc::scoped_refptr<const webrtc::RTCStatsReport>&)> callback);

 private:
  rtc::scoped_refptr<webrtc::MediaStreamInterface> getLocalStream();
  rtc::scoped_refptr<webrtc::AudioTrackInterface> getLocalAudioTrack();
  rtc::scoped_refptr<webrtc::VideoTrackInterface> getLocalVideoTrack();
  bool setMediaEnabled(
      rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track,
      bool enabled);
  bool isMediaEnabled(
      rtc::scoped_refptr<webrtc::MediaStreamTrackInterface> track);

  RTCMessageSender* sender_;
  std::unique_ptr<PeerConnectionObserver> observer_;
  rtc::scoped_refptr<webrtc::PeerConnectionInterface> connection_;
};

}  // namespace sora

#endif
