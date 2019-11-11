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
    Sink(webrtc::VideoTrackInterface* track) : track_(track) {
      ptrid_ = IdPointer::Instance().Register(this);
      track_->AddOrUpdateSink(this, rtc::VideoSinkWants());
    }
    ~Sink() { IdPointer::Instance().Unregister(ptrid_); }
    ptrid_t GetSinkID() const { return ptrid_; }

   private:
    rtc::scoped_refptr<webrtc::VideoFrameBuffer> GetFrameBuffer() {
      std::lock_guard<std::mutex> guard(mutex_);
      return frame_buffer_;
    }
    void SetFrameBuffer(rtc::scoped_refptr<webrtc::VideoFrameBuffer> v) {
      std::lock_guard<std::mutex> guard(mutex_);
      frame_buffer_ = v;
    }

   public:
    void OnFrame(const webrtc::VideoFrame& frame) override {
      SetFrameBuffer(frame.video_frame_buffer());
    }

    static void UNITY_INTERFACE_API TextureUpdateCallback(int eventID,
                                                          void* data) {
      auto event = static_cast<UnityRenderingExtEventType>(eventID);

      if (event == kUnityRenderingExtEventUpdateTextureBeginV2) {
        auto params =
            reinterpret_cast<UnityRenderingExtTextureUpdateParamsV2*>(data);
        Sink* p = (Sink*)IdPointer::Instance().Lookup(params->userData);
        if (p == nullptr) {
          return;
        }
        auto video_frame_buffer = p->GetFrameBuffer();
        if (!video_frame_buffer) {
          return;
        }

        // UpdateTextureBegin: Generate and return texture image data.
        rtc::scoped_refptr<webrtc::I420Buffer> i420_buffer =
            webrtc::I420Buffer::Create(params->width, params->height);
        i420_buffer->ScaleFrom(*video_frame_buffer->ToI420());
        delete[] p->temp_buf_;
        p->temp_buf_ = new uint8_t[params->width * params->height * 4];
        libyuv::I420ToABGR(i420_buffer->DataY(), i420_buffer->StrideY(),
                           i420_buffer->DataU(), i420_buffer->StrideU(),
                           i420_buffer->DataV(), i420_buffer->StrideV(),
                           p->temp_buf_, params->width * 4, params->width,
                           params->height);
        params->texData = p->temp_buf_;
      } else if (event == kUnityRenderingExtEventUpdateTextureEndV2) {
        auto params =
            reinterpret_cast<UnityRenderingExtTextureUpdateParamsV2*>(data);
        Sink* p = (Sink*)IdPointer::Instance().Lookup(params->userData);
        if (p == nullptr) {
          return;
        }
        delete[] p->temp_buf_;
        p->temp_buf_ = nullptr;
      }
    }
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
                std::function<void(ptrid_t)> on_remove_track)
      : on_add_track_(on_add_track), on_remove_track_(on_remove_track) {}

  void AddTrack(webrtc::VideoTrackInterface* track) override {
    std::unique_ptr<Sink> sink(new Sink(track));
    auto sink_id = sink->GetSinkID();
    sinks_.push_back(std::make_pair(track, std::move(sink)));
    on_add_track_(sink_id);
  }

  void RemoveTrack(webrtc::VideoTrackInterface* track) override {
    auto f = [track](const VideoSinkVector::value_type& sink) {
      return sink.first == track;
    };
    auto it = std::find_if(sinks_.begin(), sinks_.end(), f);
    if (it == sinks_.end()) {
      return;
    }
    auto sink_id = it->second->GetSinkID();
    sinks_.erase(std::remove_if(sinks_.begin(), sinks_.end(), f), sinks_.end());
    on_remove_track_(sink_id);
  }
};

}  // namespace sora
#endif  // SORA_UNITY_RENDERER_H_INCLUDED