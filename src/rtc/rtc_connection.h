#ifndef SORA_RTC_CONNECTION_H_
#define SORA_RTC_CONNECTION_H_

// WebRTC
#include <api/peer_connection_interface.h>

#include "peer_connection_observer.h"

namespace sora {

class RTCConnection {
 public:
  RTCConnection(RTCMessageSender* sender,
                std::unique_ptr<PeerConnectionObserver> observer,
                rtc::scoped_refptr<webrtc::PeerConnectionInterface> connection)
      : sender_(sender),
        observer_(std::move(observer)),
        connection_(connection) {}
  ~RTCConnection();

  typedef std::function<void(webrtc::SessionDescriptionInterface*)>
      OnCreateSuccessFunc;
  typedef std::function<void(webrtc::RTCError)> OnCreateFailureFunc;

  typedef std::function<void()> OnSetSuccessFunc;
  typedef std::function<void(webrtc::RTCError)> OnSetFailureFunc;

  void CreateOffer(OnCreateSuccessFunc on_success = nullptr,
                   OnCreateFailureFunc on_failure = nullptr);
  void SetOffer(const std::string sdp,
                OnSetSuccessFunc on_success = nullptr,
                OnSetFailureFunc on_failure = nullptr);
  void CreateAnswer(OnCreateSuccessFunc on_success = nullptr,
                    OnCreateFailureFunc on_failure = nullptr);
  void SetAnswer(const std::string sdp,
                 OnSetSuccessFunc on_success = nullptr,
                 OnSetFailureFunc on_failure = nullptr);
  void AddIceCandidate(const std::string sdp_mid,
                       const int sdp_mlineindex,
                       const std::string sdp);

  void GetStats(
      std::function<void(
          const rtc::scoped_refptr<const webrtc::RTCStatsReport>&)> callback);

  void SetEncodingParameters(
      std::string mid,
      std::vector<webrtc::RtpEncodingParameters> encodings);
  void ResetEncodingParameters();

  rtc::scoped_refptr<webrtc::PeerConnectionInterface> GetConnection() const;

 private:
  RTCMessageSender* sender_;
  std::unique_ptr<PeerConnectionObserver> observer_;
  rtc::scoped_refptr<webrtc::PeerConnectionInterface> connection_;
  std::vector<webrtc::RtpEncodingParameters> encodings_;
  std::string mid_;
};

}  // namespace sora

#endif