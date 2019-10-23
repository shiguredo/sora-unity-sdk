#include <iostream>
#include "rtc_base/logging.h"

#include "observer.h"

namespace sora {

void PeerConnectionObserver::OnTrack(
    rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) {
  if (receiver_ == nullptr) {
    return;
  }
  auto track = transceiver->receiver()->track();
  if (track->kind() != webrtc::MediaStreamTrackInterface::kVideoKind) {
    return;
  }
  auto video_track = static_cast<webrtc::VideoTrackInterface*>(track.get());
  receiver_->AddTrack(video_track);
  video_tracks_.push_back(video_track);
}

void PeerConnectionObserver::OnRemoveTrack(
    rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) {
  if (receiver_ == nullptr) {
    return;
  }
  auto track = receiver->track();
  if (track->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
    webrtc::VideoTrackInterface* video_track =
        static_cast<webrtc::VideoTrackInterface*>(track.get());
    video_tracks_.erase(
        std::remove_if(video_tracks_.begin(), video_tracks_.end(),
                       [video_track](const webrtc::VideoTrackInterface* track) {
                         return track == video_track;
                       }),
        video_tracks_.end());
    receiver_->RemoveTrack(video_track);
  }
}

void PeerConnectionObserver::ClearAllRegisteredTracks() {
  for (webrtc::VideoTrackInterface* video_track : video_tracks_) {
    receiver_->RemoveTrack(video_track);
  }
  video_tracks_.clear();
}

void PeerConnectionObserver::OnIceConnectionChange(
    webrtc::PeerConnectionInterface::IceConnectionState new_state) {
  sender_->onIceConnectionStateChange(new_state);
}

void PeerConnectionObserver::OnIceCandidate(
    const webrtc::IceCandidateInterface* candidate) {
  std::string sdp;
  if (candidate->ToString(&sdp)) {
    if (sender_ != nullptr) {
      sender_->onIceCandidate(candidate->sdp_mid(),
                              candidate->sdp_mline_index(), sdp);
    }
  } else {
    RTC_LOG(LS_ERROR) << "Failed to serialize candidate";
  }
}

void CreateSessionDescriptionObserver::OnSuccess(
    webrtc::SessionDescriptionInterface* desc) {
  std::string sdp;
  desc->ToString(&sdp);
  RTC_LOG(LS_INFO) << "Created session description : " << sdp;
  _connection->SetLocalDescription(
      SetSessionDescriptionObserver::Create(desc->GetType(), sender_), desc);
  if (sender_ != nullptr) {
    sender_->onCreateDescription(desc->GetType(), sdp);
  }
}

void CreateSessionDescriptionObserver::OnFailure(webrtc::RTCError error) {
  RTC_LOG(LS_ERROR) << "Failed to create session description : "
                    << ToString(error.type()) << ": " << error.message();
}

void SetSessionDescriptionObserver::OnSuccess() {
  RTC_LOG(LS_INFO) << "Set local description success!";
  if (sender_ != nullptr) {
    sender_->onSetDescription(_type);
  }
}

void SetSessionDescriptionObserver::OnFailure(webrtc::RTCError error) {
  RTC_LOG(LS_ERROR) << "Failed to set local description : "
                    << ToString(error.type()) << ": " << error.message();
}

}  // namespace sora
