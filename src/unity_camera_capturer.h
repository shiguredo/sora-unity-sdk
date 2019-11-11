#ifndef SORA_UNITY_CAMERA_CAPTURER_H_INCLUDED
#define SORA_UNITY_CAMERA_CAPTURER_H_INCLUDED

// WebRTC
#include "api/media_stream_interface.h"
#include "api/scoped_refptr.h"
#include "api/video/i420_buffer.h"
#include "libyuv.h"
#include "rtc_base/logging.h"
#include "rtc_base/ref_counted_object.h"
#include "system_wrappers/include/clock.h"

// sora
#include "rtc/scalable_track_source.h"
#include "unity_context.h"

namespace sora {

class UnityCameraCapturer : public sora::ScalableVideoTrackSource,
                            public rtc::VideoSinkInterface<webrtc::VideoFrame> {
  UnityContext* context_;
  void* camera_texture_;
  void* frame_texture_;
  int width_;
  int height_;
  webrtc::Clock* clock_ = webrtc::Clock::GetRealTimeClock();

 public:
  static rtc::scoped_refptr<UnityCameraCapturer> Create(
      UnityContext* context,
      void* unity_camera_texture,
      int width,
      int height);

  void OnRender();

  void OnFrame(const webrtc::VideoFrame& frame) override;

 private:
  bool Init(UnityContext* context,
            void* unity_camera_texture,
            int width,
            int height);
};

}  // namespace sora

#endif  // SORA_UNITY_CAMERA_CAPTURER_H_INCLUDED
