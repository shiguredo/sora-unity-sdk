#ifndef SORA_VIDEO_TRACK_RECEIVER_H_
#define SORA_VIDEO_TRACK_RECEIVER_H_

#include "api/media_stream_interface.h"

class VideoTrackReceiver {
 public:
  virtual void AddTrack(webrtc::VideoTrackInterface* track) = 0;
  virtual void RemoveTrack(webrtc::VideoTrackInterface* track) = 0;
};

#endif  // SORA_VIDEO_TRACK_RECEIVER_H_