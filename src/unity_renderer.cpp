#include "unity_renderer.h"

#include "rtc_base/logging.h"

namespace sora {

// UnityRenderer::Sink

UnityRenderer::Sink::Sink(webrtc::VideoTrackInterface* track) : track_(track) {
  ptrid_ = IdPointer::Instance().Register(this);
  track_->AddOrUpdateSink(this, rtc::VideoSinkWants());
}
UnityRenderer::Sink::~Sink() {
  IdPointer::Instance().Unregister(ptrid_);
}
ptrid_t UnityRenderer::Sink::GetSinkID() const {
  return ptrid_;
}

rtc::scoped_refptr<webrtc::VideoFrameBuffer>
UnityRenderer::Sink::GetFrameBuffer() {
  std::lock_guard<std::mutex> guard(mutex_);
  return frame_buffer_;
}
void UnityRenderer::Sink::SetFrameBuffer(
    rtc::scoped_refptr<webrtc::VideoFrameBuffer> v) {
  std::lock_guard<std::mutex> guard(mutex_);
  frame_buffer_ = v;
}

void UnityRenderer::Sink::OnFrame(const webrtc::VideoFrame& frame) {
  //RTC_LOG(LS_INFO) << "OnFrame: width=" << frame.width()
  //                 << " height=" << frame.height() << " size=" << frame.size()
  //                 << " timestamp=" << frame.timestamp()
  //                 << " rotation=" << (int)frame.rotation()
  //                 << " videowidth=" << frame.video_frame_buffer()->width()
  //                 << " videoheight=" << frame.video_frame_buffer()->height()
  //                 << " videotype=" << (int)frame.video_frame_buffer()->type();

  // kNative の場合は別スレッドで変換が出来ない可能性が高いため、
  // ここで I420 に変換して渡す。
  if (frame.video_frame_buffer()->type() ==
      webrtc::VideoFrameBuffer::Type::kNative) {
    SetFrameBuffer(frame.video_frame_buffer()->ToI420());
  } else {
    SetFrameBuffer(frame.video_frame_buffer());
  }
}

void UnityRenderer::Sink::TextureUpdateCallback(int eventID, void* data) {
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
    libyuv::I420ToABGR(
        i420_buffer->DataY(), i420_buffer->StrideY(), i420_buffer->DataU(),
        i420_buffer->StrideU(), i420_buffer->DataV(), i420_buffer->StrideV(),
        p->temp_buf_, params->width * 4, params->width, params->height);
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

UnityRenderer::UnityRenderer(std::function<void(ptrid_t)> on_add_track,
                             std::function<void(ptrid_t)> on_remove_track)
    : on_add_track_(on_add_track), on_remove_track_(on_remove_track) {}

void UnityRenderer::AddTrack(webrtc::VideoTrackInterface* track) {
  std::unique_ptr<Sink> sink(new Sink(track));
  auto sink_id = sink->GetSinkID();
  sinks_.push_back(std::make_pair(track, std::move(sink)));
  on_add_track_(sink_id);
}

void UnityRenderer::RemoveTrack(webrtc::VideoTrackInterface* track) {
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

}  // namespace sora
