#include "sora.h"

namespace sora {

Sora::Sora(UnityContext* context) : ioc_(1), context_(context) {
  ptrid_ = IdPointer::Instance().Register(this);
}

Sora::~Sora() {
  RTC_LOG(LS_INFO) << "Sora object destroy started";

  IdPointer::Instance().Unregister(ptrid_);

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
void Sora::SetOnAddTrack(std::function<void(ptrid_t)> on_add_track) {
  on_add_track_ = on_add_track;
}
void Sora::SetOnRemoveTrack(std::function<void(ptrid_t)> on_remove_track) {
  on_remove_track_ = on_remove_track;
}
void Sora::SetOnNotify(std::function<void(std::string)> on_notify) {
  on_notify_ = std::move(on_notify);
}

void Sora::DispatchEvents() {
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

bool Sora::Connect(std::string signaling_url,
                   std::string channel_id,
                   std::string metadata,
                   bool downstream,
                   bool multistream,
                   int capturer_type,
                   void* unity_camera_texture,
                   int video_width,
                   int video_height,
                   bool unity_audio_input) {
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

  RTC_LOG(LS_INFO) << "Sora::Connect signaling_url=" << signaling_url_
                   << " channel_id=" << channel_id_ << " metadata=" << metadata
                   << " downstream=" << downstream
                   << " multistream=" << multistream
                   << " capturer_type=" << capturer_type
                   << " unity_camera_texture=0x" << unity_camera_texture
                   << " video_width=" << video_width
                   << " video_height=" << video_height;

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

  std::function<rtc::scoped_refptr<webrtc::AudioDeviceModule>(
      rtc::scoped_refptr<webrtc::AudioDeviceModule>,
      webrtc::TaskQueueFactory * task_queue_factory)>
      create_adm;
  if (unity_audio_input) {
    create_adm = [this](rtc::scoped_refptr<webrtc::AudioDeviceModule> adm,
                        webrtc::TaskQueueFactory* task_queue_factory) {
      adm_ = UnityAudioDevice::Create(adm, task_queue_factory);
      return adm_;
    };
  }

  if (!downstream) {
    // 送信側は capturer を設定する。playout の設定はしない
    rtc::scoped_refptr<ScalableVideoTrackSource> capturer;

    if (capturer_type == 0) {
      // 実カメラ（デバイス）を使う
      // TODO(melpon): framerate をちゃんと設定する
#ifdef __APPLE__
      capturer = MacCapturer::Create(video_width, video_height, 30, "");
#else
      capturer = DeviceVideoCapturer::Create(video_width, video_height, 30);
#endif
    } else {
      // Unity のカメラからの映像を使う
      unity_camera_capturer_ = UnityCameraCapturer::Create(
          &UnityContext::Instance(), unity_camera_texture, video_width,
          video_height);
      capturer = unity_camera_capturer_;
    }
    RTCManagerConfig config;
    config.no_playout = true;
    rtc_manager_.reset(new RTCManager(config, std::move(capturer),
                                      renderer_.get(), create_adm));
  } else {
    // 受信側は capturer を作らず、video, recording の設定もしない
    RTCManagerConfig config;
    config.no_recording = true;
    config.no_video = true;
    rtc_manager_.reset(
        new RTCManager(config, nullptr, renderer_.get(), create_adm));
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
    if (!metadata.empty()) {
      auto md = nlohmann::json::parse(metadata, nullptr, false);
      if (md.type() == nlohmann::json::value_t::discarded) {
        RTC_LOG(LS_WARNING) << "Invalid JSON: metadata=" << metadata;
      } else {
        config.metadata = md;
      }
    }

    signaling_ = SoraSignaling::Create(
        ioc_, rtc_manager_.get(), config, [this](std::string json) {
          std::lock_guard<std::mutex> guard(event_mutex_);
          event_queue_.push_back([this, json = std::move(json)]() {
            // ここは Unity スレッドから呼ばれる
            if (on_notify_) {
              on_notify_(std::move(json));
            }
          });
        });
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

void Sora::RenderCallbackStatic(int event_id) {
  auto sora = (sora::Sora*)IdPointer::Instance().Lookup(event_id);
  if (sora == nullptr) {
    return;
  }

  sora->RenderCallback();
}
int Sora::GetRenderCallbackEventID() const {
  return ptrid_;
}

void Sora::RenderCallback() {
  if (!unity_camera_capturer_) {
    return;
  }
  unity_camera_capturer_->OnRender();
}
void Sora::ProcessAudio(const void* p, int offset, int samples) {
  if (!adm_) {
    return;
  }
  // 今のところステレオデータを渡すようにしてるので2倍する
  adm_->ProcessAudioData((const float*)p + offset, samples * 2);
}

}  // namespace sora
