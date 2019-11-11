#ifndef SORA_UNITY_RENDERER_H_INCLUDED
#define SORA_UNITY_RENDERER_H_INCLUDED

// webrtc
#include "api/video/i420_buffer.h"
#include "libyuv.h"

// sora
#include "id_pointer.h"
#include "rtc/video_track_receiver.h"
#include "unity/IUnityRenderingExtensions.h"

namespace sora {

class UnityRenderer : public VideoTrackReceiver {
 public:
  class Sink : public rtc::VideoSinkInterface<webrtc::VideoFrame> {
    webrtc::VideoTrackInterface* track_;
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
  std::function<void(ptrid_t)> on_add_track_;
  std::function<void(ptrid_t)> on_remove_track_;

 public:
  UnityRenderer(std::function<void(ptrid_t)> on_add_track,
                std::function<void(ptrid_t)> on_remove_track);

  void AddTrack(webrtc::VideoTrackInterface* track) override;
  void RemoveTrack(webrtc::VideoTrackInterface* track) override;
};

}  // namespace sora
#endif  // SORA_UNITY_RENDERER_H_INCLUDED
