#include "sora.h"

#include "api/task_queue/default_task_queue_factory.h"
#include "modules/audio_device/include/audio_device_factory.h"

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
                   std::string video_capturer_device,
                   int video_width,
                   int video_height,
                   bool unity_audio_input,
                   std::string audio_recording_device,
                   std::string audio_playout_device) {
  signaling_url_ = std::move(signaling_url);
  channel_id_ = std::move(channel_id);

  RTC_LOG(LS_INFO) << "Sora::Connect signaling_url=" << signaling_url_
                   << " channel_id=" << channel_id_ << " metadata=" << metadata
                   << " downstream=" << downstream
                   << " multistream=" << multistream
                   << " capturer_type=" << capturer_type
                   << " unity_camera_texture=0x" << unity_camera_texture
                   << " video_capturer_device=" << video_capturer_device
                   << " video_width=" << video_width
                   << " video_height=" << video_height
                   << " unity_audio_input=" << unity_audio_input
                   << " audio_recording_device=" << audio_recording_device
                   << " audio_playout_device=" << audio_playout_device;

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

  auto task_queue_factory = webrtc::CreateDefaultTaskQueueFactory();
  auto adm =
      CreateADM(task_queue_factory.get(), false, unity_audio_input,
                audio_recording_device, audio_playout_device, unity_adm_);
  if (!adm) {
    return false;
  }

  if (!downstream) {
    // 送信側は capturer を設定する。playout の設定はしない
    rtc::scoped_refptr<ScalableVideoTrackSource> capturer = CreateVideoCapturer(
        capturer_type, unity_camera_texture, video_capturer_device, video_width,
        video_height, unity_camera_capturer_);
    if (!capturer) {
      return false;
    }

    RTCManagerConfig config;
    config.no_playout = true;
    config.audio_recording_device = audio_recording_device;
    config.audio_playout_device = audio_playout_device;
    rtc_manager_.reset(new RTCManager(config, std::move(capturer),
                                      renderer_.get(), adm,
                                      std::move(task_queue_factory)));
  } else {
    // 受信側は capturer を作らず、video, recording の設定もしない
    RTCManagerConfig config;
    config.no_recording = true;
    config.no_video = true;
    config.audio_recording_device = audio_recording_device;
    config.audio_playout_device = audio_playout_device;
    rtc_manager_.reset(new RTCManager(config, nullptr, renderer_.get(), adm,
                                      std::move(task_queue_factory)));
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
  if (!unity_adm_) {
    return;
  }
  // 今のところステレオデータを渡すようにしてるので2倍する
  unity_adm_->ProcessAudioData((const float*)p + offset, samples * 2);
}

rtc::scoped_refptr<webrtc::AudioDeviceModule> Sora::CreateADM(
    webrtc::TaskQueueFactory* task_queue_factory,
    bool dummy_audio,
    bool unity_audio_input,
    std::string audio_recording_device,
    std::string audio_playout_device,
    rtc::scoped_refptr<UnityAudioDevice>& unity_adm) {
  rtc::scoped_refptr<webrtc::AudioDeviceModule> adm;

  if (dummy_audio) {
    adm = webrtc::AudioDeviceModule::Create(
        webrtc::AudioDeviceModule::kDummyAudio, task_queue_factory);
  } else {
#ifdef _WIN32
    adm = webrtc::CreateWindowsCoreAudioAudioDeviceModule(task_queue_factory);
#else
    adm = webrtc::AudioDeviceModule::Create(
        webrtc::AudioDeviceModule::kPlatformDefaultAudio, task_queue_factory);
#endif
  }

  if (unity_audio_input) {
    unity_adm = UnityAudioDevice::Create(adm, task_queue_factory);
    return unity_adm;
  } else {
    return adm;
  }
}

rtc::scoped_refptr<ScalableVideoTrackSource> Sora::CreateVideoCapturer(
    int capturer_type,
    void* unity_camera_texture,
    std::string video_capturer_device,
    int video_width,
    int video_height,
    rtc::scoped_refptr<UnityCameraCapturer>& unity_camera_capturer) {
  // 送信側は capturer を設定する。playout の設定はしない
  rtc::scoped_refptr<ScalableVideoTrackSource> capturer;

  if (capturer_type == 0) {
    // 実カメラ（デバイス）を使う
    // TODO(melpon): framerate をちゃんと設定する
#ifdef __APPLE__
    return capturer = MacCapturer::Create(video_width, video_height, 30,
                                          video_capturer_device);
#else
    return DeviceVideoCapturer::Create(video_width, video_height, 30,
                                       video_capturer_device);
#endif
  } else {
    // Unity のカメラからの映像を使う
    unity_camera_capturer = UnityCameraCapturer::Create(
        &UnityContext::Instance(), unity_camera_texture, video_width,
        video_height);
    return unity_camera_capturer;
  }
}

}  // namespace sora
