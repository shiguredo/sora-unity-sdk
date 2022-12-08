#ifndef SORA_UNITY_SDK_UNITY_RENDERER_H_INCLUDED
#define SORA_UNITY_SDK_UNITY_RENDERER_H_INCLUDED

// webrtc
#include <api/media_stream_interface.h>
#include <api/video/i420_buffer.h>
#include <api/video/video_frame.h>
#include <api/video/video_sink_interface.h>
#include <libyuv.h>

// sora
#include "id_pointer.h"
#include "unity/IUnityRenderingExtensions.h"

namespace sora_unity_sdk {

class UnityRenderer {
 public:
  class Sink : public rtc::VideoSinkInterface<webrtc::VideoFrame> {
    rtc::scoped_refptr<webrtc::VideoTrackInterface> track_;
    ptrid_t ptrid_;
    std::mutex mutex_;
    rtc::scoped_refptr<webrtc::VideoFrameBuffer> frame_buffer_;
    uint8_t* temp_buf_ = nullptr;

   public:
    Sink(webrtc::VideoTrackInterface* track);
    ~Sink();
    ptrid_t GetSinkID() const;

   private:
    rtc::scoped_refptr<webrtc::VideoFrameBuffer> GetFrameBuffer();
    void SetFrameBuffer(rtc::scoped_refptr<webrtc::VideoFrameBuffer> v);

   public:
    void OnFrame(const webrtc::VideoFrame& frame) override;
    static void UNITY_INTERFACE_API TextureUpdateCallback(int eventID,
                                                          void* data);
  };

 private:
  typedef std::vector<
      std::pair<webrtc::VideoTrackInterface*, std::unique_ptr<Sink>>>
      VideoSinkVector;
  VideoSinkVector sinks_;

 public:
  ptrid_t AddTrack(webrtc::VideoTrackInterface* track);
  ptrid_t RemoveTrack(webrtc::VideoTrackInterface* track);
};

}  // namespace sora_unity_sdk

#endif
