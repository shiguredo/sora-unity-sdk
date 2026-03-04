#include "unity_renderer.h"

#include <thread>

// libwebrtc
#include <rtc_base/logging.h>

namespace sora_unity_sdk {

// UnityRenderer::Sink

UnityRenderer::Sink::Sink(webrtc::VideoTrackInterface* track) : track_(track) {
  RTC_LOG(LS_INFO) << "[" << (void*)this << "] Sink::Sink";
  deleting_ = false;
  updating_ = false;
  ptrid_ = IdPointer::Instance().Register(this);
  track_->AddOrUpdateSink(this, webrtc::VideoSinkWants());
}
UnityRenderer::Sink::~Sink() {
  RTC_LOG(LS_INFO) << "[" << (void*)this << "] Sink::~Sink";
  // デストラクタとテクスチャのアップデートが同時に走る可能性があるので、
  // 削除中フラグを立てて、テクスチャのアップデートが終わるまで待つ。
  deleting_ = true;
  while (updating_) {
    // RTC_LOG(LS_INFO) << "[" << (void*)this << "] Sink::~Sink waiting...";
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  track_->RemoveSink(this);
  IdPointer::Instance().Unregister(ptrid_);
}
ptrid_t UnityRenderer::Sink::GetSinkID() const {
  return ptrid_;
}
void UnityRenderer::Sink::SetTrack(webrtc::VideoTrackInterface* track) {
  track_->RemoveSink(this);
  track->AddOrUpdateSink(this, webrtc::VideoSinkWants());
  track_ = track;
}

webrtc::scoped_refptr<webrtc::VideoFrameBuffer>
UnityRenderer::Sink::GetFrameBuffer() {
  std::lock_guard<std::mutex> guard(mutex_);
  return frame_buffer_;
}
void UnityRenderer::Sink::SetFrameBuffer(
    webrtc::scoped_refptr<webrtc::VideoFrameBuffer> v) {
  std::lock_guard<std::mutex> guard(mutex_);
  frame_buffer_ = v;
}

void UnityRenderer::Sink::OnFrame(const webrtc::VideoFrame& frame) {
  webrtc::scoped_refptr<webrtc::VideoFrameBuffer> frame_buffer =
      frame.video_frame_buffer();

  // kNative の場合は別スレッドで変換が出来ない可能性が高いため、
  // ここで I420 に変換する。
  if (frame_buffer->type() == webrtc::VideoFrameBuffer::Type::kNative) {
    frame_buffer = frame_buffer->ToI420();
  }

  SetFrameBuffer(frame_buffer);
}

void UnityRenderer::Sink::TextureUpdateCallback(int eventID, void* data) {
  auto event = static_cast<UnityRenderingExtEventType>(eventID);

  if (event == kUnityRenderingExtEventUpdateTextureBeginV2) {
    auto params =
        reinterpret_cast<UnityRenderingExtTextureUpdateParamsV2*>(data);
    Sink* p = (Sink*)IdPointer::Instance().Lookup(params->userData);
    if (p == nullptr) {
      //RTC_LOG(LS_INFO) << "[" << (void*)p
      //                 << "] Sink::TextureUpdateCallback Begin Sink is null";
      return;
    }
    // TODO(melpon): p を取得した直後、updating_ = true にするまでの間に Sink が削除されたら
    // セグフォしてしまうので、問題になるようなら Lookup の時点でロックを獲得する必要がある

    if (p->deleting_) {
      //RTC_LOG(LS_INFO) << "[" << (void*)p
      //                 << "] Sink::TextureUpdateCallback Begin deleting";
      p->updating_ = false;
      return;
    }
    p->updating_ = true;
    //RTC_LOG(LS_INFO) << "[" << (void*)p
    //                 << "] Sink::TextureUpdateCallback Begin Start";
    auto video_frame_buffer = p->GetFrameBuffer();
    if (!video_frame_buffer) {
      return;
    }

    // UpdateTextureBegin: Generate and return texture image data.
    webrtc::scoped_refptr<webrtc::I420Buffer> i420_buffer =
        webrtc::I420Buffer::Create(params->width, params->height);
    i420_buffer->ScaleFrom(*video_frame_buffer->ToI420());
    delete[] p->temp_buf_;
    p->temp_buf_ = new uint8_t[params->width * params->height * 4];
    libyuv::I420ToABGR(
        i420_buffer->DataY(), i420_buffer->StrideY(), i420_buffer->DataU(),
        i420_buffer->StrideU(), i420_buffer->DataV(), i420_buffer->StrideV(),
        p->temp_buf_, params->width * 4, params->width, params->height);
    params->texData = p->temp_buf_;
    //RTC_LOG(LS_INFO) << "[" << (void*)p
    //                 << "] Sink::TextureUpdateCallback Begin Finish";
  } else if (event == kUnityRenderingExtEventUpdateTextureEndV2) {
    auto params =
        reinterpret_cast<UnityRenderingExtTextureUpdateParamsV2*>(data);
    Sink* p = (Sink*)IdPointer::Instance().Lookup(params->userData);
    if (p == nullptr) {
      //RTC_LOG(LS_INFO) << "[" << (void*)p
      //                 << "] Sink::TextureUpdateCallback End Sink is null";
      return;
    }
    //RTC_LOG(LS_INFO) << "[" << (void*)p << "] Sink::TextureUpdateCallback End";
    delete[] p->temp_buf_;
    p->temp_buf_ = nullptr;
    p->updating_ = false;
  }
}

ptrid_t UnityRenderer::AddTrack(webrtc::VideoTrackInterface* track) {
  RTC_LOG(LS_INFO) << "UnityRenderer::AddTrack";
  std::unique_ptr<Sink> sink(new Sink(track));
  auto sink_id = sink->GetSinkID();
  sinks_.push_back(std::make_pair(track, std::move(sink)));
  return sink_id;
}

ptrid_t UnityRenderer::RemoveTrack(webrtc::VideoTrackInterface* track) {
  RTC_LOG(LS_INFO) << "UnityRenderer::RemoveTrack";
  auto f = [track](const VideoSinkVector::value_type& sink) {
    return sink.first == track;
  };
  auto it = std::find_if(sinks_.begin(), sinks_.end(), f);
  if (it == sinks_.end()) {
    return 0;
  }
  auto sink_id = it->second->GetSinkID();
  sinks_.erase(std::remove_if(sinks_.begin(), sinks_.end(), f), sinks_.end());
  return sink_id;
}

void UnityRenderer::ReplaceTrack(webrtc::VideoTrackInterface* oldTrack,
                                 webrtc::VideoTrackInterface* newTrack) {
  RTC_LOG(LS_INFO) << "UnityRenderer::ReplaceTrack";
  auto it = std::find_if(sinks_.begin(), sinks_.end(),
                         [oldTrack](const VideoSinkVector::value_type& sink) {
                           return sink.first == oldTrack;
                         });
  if (it == sinks_.end()) {
    return;
  }
  it->first = newTrack;
  it->second->SetTrack(newTrack);
}

webrtc::VideoTrackInterface* UnityRenderer::GetVideoTrackFromVideoSinkId(
    ptrid_t video_sink_id) const {
  auto it =
      std::find_if(sinks_.begin(), sinks_.end(),
                   [video_sink_id](const VideoSinkVector::value_type& sink) {
                     return sink.second->GetSinkID() == video_sink_id;
                   });
  if (it == sinks_.end()) {
    return nullptr;
  }
  return it->first;
}

ptrid_t UnityRenderer::GetVideoSinkId(
    webrtc::VideoTrackInterface* track) const {
  auto it = std::find_if(sinks_.begin(), sinks_.end(),
                         [track](const VideoSinkVector::value_type& sink) {
                           return sink.first == track;
                         });
  if (it == sinks_.end()) {
    return 0;
  }
  return it->second->GetSinkID();
}

}  // namespace sora_unity_sdk
