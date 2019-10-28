#include <memory>
#include <string>
#include <thread>

#include <boost/asio/io_context.hpp>

#include "api/video/i420_buffer.h"
#include "rtc/device_video_capturer.h"
#include "rtc/rtc_manager.h"
#include "rtc_base/log_sinks.h"
#include "third_party/libyuv/include/libyuv.h"

#include "unity/IUnityRenderingExtensions.h"

#include "sora_signaling.h"
#include "unity.h"

namespace sora {

// TextureUpdateCallback のユーザデータが 32bit 整数しか扱えないので、
// ID からポインタに変換する仕組みを用意する
class IdPointer {
  std::mutex mutex_;
  ptrid_t counter_ = 1;
  std::map<ptrid_t, void*> map_;

 public:
  static IdPointer& Instance() {
    static IdPointer ip;
    return ip;
  }
  ptrid_t Register(void* p) {
    std::lock_guard<std::mutex> guard(mutex_);
    map_[counter_] = p;
    return counter_++;
  }
  void Unregister(ptrid_t id) {
    std::lock_guard<std::mutex> guard(mutex_);
    map_.erase(id);
  }
  void* Lookup(ptrid_t id) {
    std::lock_guard<std::mutex> guard(mutex_);
    auto it = map_.find(id);
    if (it == map_.end()) {
      return nullptr;
    }
    return it->second;
  }
};

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

    static void TextureUpdateCallback(int eventID, void* data) {
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

class Sora {
  boost::asio::io_context ioc_;
  std::string signaling_url_;
  std::string channel_id_;
  std::unique_ptr<RTCManager> rtc_manager_;
  std::shared_ptr<SoraSignaling> signaling_;
  std::unique_ptr<rtc::Thread> thread_;
  std::unique_ptr<rtc::FileRotatingLogSink> log_sink_;
  std::unique_ptr<UnityRenderer> renderer_;
  std::function<void(ptrid_t)> on_add_track_;
  std::function<void(ptrid_t)> on_remove_track_;

  std::mutex event_mutex_;
  std::deque<std::function<void()>> event_queue_;

 public:
  Sora() : ioc_(1) {}

  ~Sora() {
    RTC_LOG(LS_INFO) << "Sora object destroy started";
    if (signaling_) {
      signaling_->release();
    }
    signaling_.reset();
    rtc_manager_.reset();
    renderer_.reset();
    ioc_.stop();
    if (thread_) {
      thread_->Stop();
    }
    RTC_LOG(LS_INFO) << "Sora object destroy finished";
    rtc::LogMessage::RemoveLogToStream(log_sink_.get());
    log_sink_.reset();
  }
  void SetOnAddTrack(std::function<void(ptrid_t)> on_add_track) {
    on_add_track_ = on_add_track;
  }
  void SetOnRemoveTrack(std::function<void(ptrid_t)> on_remove_track) {
    on_remove_track_ = on_remove_track;
  }
  void DispatchEvents() {
    while (!event_queue_.empty()) {
      std::function<void()> f;
      {
        std::lock_guard<std::mutex> guard(event_mutex_);
        f = std::move(event_queue_.front());
        event_queue_.pop_front();
      }
      f();
    }
  }

  bool Connect(std::string signaling_url,
               std::string channel_id,
               bool downstream,
               bool multistream) {
    signaling_url_ = std::move(signaling_url);
    channel_id_ = std::move(channel_id);

    const size_t kDefaultMaxLogFileSize = 10 * 1024 * 1024;
    rtc::LogMessage::LogToDebug((rtc::LoggingSeverity)rtc::LS_NONE);
    rtc::LogMessage::LogTimestamps();
    rtc::LogMessage::LogThreads();

    log_sink_.reset(new rtc::FileRotatingLogSink("./", "webrtc_logs",
                                                 kDefaultMaxLogFileSize, 10));
    if (!log_sink_->Init()) {
      RTC_LOG(LS_ERROR) << __FUNCTION__ << ": Failed to open log file";
      log_sink_.reset();
      return false;
    }

    rtc::LogMessage::AddLogToStream(log_sink_.get(), rtc::LS_INFO);

    RTC_LOG(LS_INFO) << "Log initialized";

    renderer_.reset(new UnityRenderer(
        [this](ptrid_t track_id) {
          std::lock_guard<std::mutex> guard(event_mutex_);
          event_queue_.push_back([this, track_id]() {
            // ここは Unity スレッドから呼ばれる
            if (on_add_track_) {
              on_add_track_(track_id);
            }
          });
        },
        [this](ptrid_t track_id) {
          std::lock_guard<std::mutex> guard(event_mutex_);
          event_queue_.push_back([this, track_id]() {
            // ここは Unity スレッドから呼ばれる
            if (on_remove_track_) {
              on_remove_track_(track_id);
            }
          });
        }));

    if (!downstream) {
      // 送信側は capturer を設定する。playout の設定はしない
      rtc::scoped_refptr<DeviceVideoCapturer> capturer =
          DeviceVideoCapturer::Create(640, 480, 30);
      RTCManagerConfig config;
      config.no_playout = true;
      rtc_manager_.reset(
          new RTCManager(config, std::move(capturer), renderer_.get()));
    } else {
      // 受信側は capturer を作らず、video, recording の設定もしない
      RTCManagerConfig config;
      config.no_recording = true;
      config.no_video = true;
      rtc_manager_.reset(new RTCManager(config, nullptr, renderer_.get()));
    }

    {
      RTC_LOG(LS_INFO) << "Start Signaling: url=" << signaling_url_
                       << " channel_id=" << channel_id_;
      SoraSignalingConfig config;
      config.role = downstream ? SoraSignalingConfig::Role::Downstream
                               : SoraSignalingConfig::Role::Upstream;
      config.multistream = multistream;
      config.signaling_url = signaling_url_;
      config.channel_id = channel_id_;
      signaling_ = SoraSignaling::Create(ioc_, rtc_manager_.get(), config);
      if (signaling_ == nullptr) {
        return false;
      }
      if (!signaling_->connect()) {
        return false;
      }
    }

    thread_ = rtc::Thread::Create();
    if (!thread_->SetName("Sora IO Thread", nullptr)) {
      RTC_LOG(LS_INFO) << "Failed to set thread name";
      return false;
    }
    if (!thread_->Start()) {
      RTC_LOG(LS_INFO) << "Failed to start thread";
      return false;
    }
    thread_->PostTask(RTC_FROM_HERE, [this]() {
      RTC_LOG(LS_INFO) << "io_context started";
      ioc_.run();
      RTC_LOG(LS_INFO) << "io_context finished";
    });

    return true;
  }
};

}  // namespace sora

#ifdef __cplusplus
extern "C" {
#endif

void* sora_create() {
  auto sora = std::unique_ptr<sora::Sora>(new sora::Sora());
  return sora.release();
}

void sora_set_on_add_track(void* p, track_cb_t on_add_track, void* userdata) {
  auto sora = (sora::Sora*)p;
  sora->SetOnAddTrack([on_add_track, userdata](ptrid_t track_id) {
    on_add_track(track_id, userdata);
  });
}

void sora_set_on_remove_track(void* p,
                              track_cb_t on_remove_track,
                              void* userdata) {
  auto sora = (sora::Sora*)p;
  sora->SetOnRemoveTrack([on_remove_track, userdata](ptrid_t track_id) {
    on_remove_track(track_id, userdata);
  });
}

void sora_dispatch_events(void* p) {
  auto sora = (sora::Sora*)p;
  sora->DispatchEvents();
}

int sora_connect(void* p,
                 const char* signaling_url,
                 const char* channel_id,
                 bool downstream,
                 bool multistream) {
  auto sora = (sora::Sora*)p;
  if (!sora->Connect(signaling_url, channel_id, downstream, multistream)) {
    return -1;
  }
  return 0;
}

void* sora_get_texture_update_callback() {
  return &sora::UnityRenderer::Sink::TextureUpdateCallback;
}

void sora_destroy(void* sora) {
  delete (sora::Sora*)sora;
}

#ifdef __cplusplus
}
#endif
