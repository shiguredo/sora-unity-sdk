/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#ifndef SORA_SCALABLE_VIDEO_TRACK_SOURCE_H_
#define SORA_SCALABLE_VIDEO_TRACK_SOURCE_H_

#include <stddef.h>

#include <memory>

#include "media/base/adapted_video_track_source.h"
#include "media/base/video_adapter.h"
#include "rtc_base/timestamp_aligner.h"

namespace sora {

class ScalableVideoTrackSource : public rtc::AdaptedVideoTrackSource {
 public:
  ScalableVideoTrackSource();
  virtual ~ScalableVideoTrackSource();

  bool is_screencast() const override;
  absl::optional<bool> needs_denoising() const override;
  webrtc::MediaSourceInterface::SourceState state() const override;
  bool remote() const override;
  void OnCapturedFrame(const webrtc::VideoFrame& frame);
  virtual bool useNativeBuffer() { return false; }

 private:
  rtc::TimestampAligner timestamp_aligner_;

  cricket::VideoAdapter video_adapter_;
};

}  // namespace sora

#endif  // SORA_SCALABLE_VIDEO_TRACK_SOURCE_H_
