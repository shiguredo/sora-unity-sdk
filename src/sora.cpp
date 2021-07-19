#include "sora.h"

#include "api/task_queue/default_task_queue_factory.h"
#include "modules/audio_device/include/audio_device_factory.h"

#ifdef SORA_UNITY_SDK_ANDROID
#include "sdk/android/native_api/audio_device_module/audio_device_android.h"
#include "sdk/android/native_api/jni/jvm.h"
#endif

#ifdef SORA_UNITY_SDK_ANDROID
#include "android_helper/android_capturer.h"
#include "android_helper/android_context.h"
#endif

#ifdef SORA_UNITY_SDK_IOS
#include "mac_helper/ios_audio_init.h"
#endif

namespace sora {

Sora::Sora(UnityContext* context) : context_(context) {
  ptrid_ = IdPointer::Instance().Register(this);
}

Sora::~Sora() {
  RTC_LOG(LS_INFO) << "Sora object destroy started";

  IdPointer::Instance().Unregister(ptrid_);

#if defined(SORA_UNITY_SDK_ANDROID)
  if (capturer_ != nullptr && capturer_type_ == 0) {
    static_cast<AndroidCapturer*>(capturer_.get())->Stop();
  }
#endif
  capturer_ = nullptr;
  unity_adm_ = nullptr;

  if (ioc_ != nullptr) {
    ioc_->stop();
  }
  if (thread_) {
    thread_->Stop();
    thread_.reset();
  }
  signaling_.reset();
  ioc_.reset();
  rtc_manager_.reset();
  renderer_.reset();
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
void Sora::SetOnPush(std::function<void(std::string)> on_push) {
  on_push_ = std::move(on_push);
}
void Sora::SetOnMessage(
    std::function<void(std::string, std::string)> on_message) {
  on_message_ = std::move(on_message);
}
void Sora::SetOnDisconnect(
    std::function<void(int, std::string)> on_disconnect) {
  on_disconnect_ = std::move(on_disconnect);
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

void Sora::Connect(const sora_conf::internal::ConnectConfig& cc) {
  auto on_disconnect = [this](int error_code, std::string reason) {
    std::lock_guard<std::mutex> guard(event_mutex_);
    event_queue_.push_back([this, error_code, reason = std::move(reason)]() {
      // ここは Unity スレッドから呼ばれる
      if (on_disconnect_) {
        on_disconnect_(error_code, std::move(reason));
      }
    });
  };

#if defined(SORA_UNITY_SDK_IOS)
  // iOS でマイクを使用する場合、マイクの初期化の設定をしてから DoConnect する。

  if (cc.role == "recvonly" || cc.unity_audio_input) {
    // この場合マイクを利用しないのですぐに DoConnect
    DoConnect(cc, std::move(on_disconnect));
    return;
  }

  IosAudioInit([this](std::string error) {
    if (!error.empty()) {
      RTC_LOG(LS_ERROR) << "Failed to IosAudioInit: error=" << error;
      on_disconnect((int)sora_conf::ErrorCode::INTERNAL_ERROR,
                    "Failed to IosAudioInit: error=" + error);
    }
  });
  DoConnect(cc, std::move(on_disconnect));
#else
  DoConnect(cc, std::move(on_disconnect));
#endif
}

void Sora::DoConnect(const sora_conf::internal::ConnectConfig& cc,
                     std::function<void(int, std::string)> on_disconnect) {
  signaling_url_ = std::move(cc.signaling_url);
  channel_id_ = std::move(cc.channel_id);

  RTC_LOG(LS_INFO) << "Sora::Connect " << jsonif::to_json(cc);

  if (cc.role != "sendonly" && cc.role != "recvonly" && cc.role != "sendrecv") {
    RTC_LOG(LS_ERROR) << "Invalid role: " << cc.role;
    on_disconnect((int)sora_conf::ErrorCode::INTERNAL_ERROR,
                  "Invalid role: " + cc.role);
    return;
  }

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

  std::unique_ptr<rtc::Thread> worker_thread = rtc::Thread::Create();
  worker_thread->Start();

  auto task_queue_factory = webrtc::CreateDefaultTaskQueueFactory();
  unity_adm_ = CreateADM(task_queue_factory.get(), false, cc.unity_audio_input,
                         cc.unity_audio_output, on_handle_audio_,
                         cc.audio_recording_device, cc.audio_playout_device,
                         worker_thread.get());
  if (!unity_adm_) {
    on_disconnect((int)sora_conf::ErrorCode::INTERNAL_ERROR, "ADM Init Failed");
    return;
  }

  std::unique_ptr<rtc::Thread> signaling_thread = rtc::Thread::Create();

  RTCManagerConfig config;
  config.audio_recording_device = cc.audio_recording_device;
  config.audio_playout_device = cc.audio_playout_device;
  config.simulcast = cc.simulcast;

  if (cc.role == "sendonly" || cc.role == "sendrecv") {
    // 送信側は capturer を設定する。送信のみの場合は playout の設定はしない
    rtc::scoped_refptr<rtc::AdaptedVideoTrackSource> capturer =
        CreateVideoCapturer(cc.capturer_type, (void*)cc.unity_camera_texture,
                            cc.video_capturer_device, cc.video_width,
                            cc.video_height, signaling_thread.get());
    if (!capturer) {
      on_disconnect((int)sora_conf::ErrorCode::INTERNAL_ERROR,
                    "Capturer Init Failed");
      return;
    }

    capturer_ = capturer;
    capturer_type_ = cc.capturer_type;

    config.no_playout = cc.role == "sendonly";

    config.audio_recording_device = cc.audio_recording_device;
    config.audio_playout_device = cc.audio_playout_device;
    rtc_manager_ = RTCManager::Create(
        config, std::move(capturer), renderer_.get(), unity_adm_,
        std::move(task_queue_factory), std::move(signaling_thread),
        std::move(worker_thread));
  } else {
    // 受信側は capturer を作らず、video, recording の設定もしない
    RTCManagerConfig config;
    config.no_recording = true;
    config.no_video = true;

    config.audio_recording_device = cc.audio_recording_device;
    config.audio_playout_device = cc.audio_playout_device;
    rtc_manager_ = RTCManager::Create(config, nullptr, renderer_.get(),
                                      unity_adm_, std::move(task_queue_factory),
                                      std::move(signaling_thread),
                                      std::move(worker_thread));
  }

  {
    RTC_LOG(LS_INFO) << "Start Signaling: url=" << signaling_url_
                     << " channel_id=" << channel_id_;
    SoraSignalingConfig config;
    config.unity_version = cc.unity_version;
    config.role = cc.role == "sendonly"   ? SoraSignalingConfig::Role::Sendonly
                  : cc.role == "recvonly" ? SoraSignalingConfig::Role::Recvonly
                                          : SoraSignalingConfig::Role::Sendrecv;
    config.multistream = cc.multistream;
    config.spotlight = cc.spotlight;
    config.spotlight_number = cc.spotlight_number;
    config.spotlight_focus_rid = cc.spotlight_focus_rid;
    config.spotlight_unfocus_rid = cc.spotlight_unfocus_rid;
    config.simulcast = cc.simulcast;
    config.simulcast_rid = cc.simulcast_rid;
    config.signaling_url = signaling_url_;
    config.channel_id = channel_id_;
    config.client_id = cc.client_id;
    config.video_codec_type = cc.video_codec_type;
    config.video_bit_rate = cc.video_bit_rate;
    config.audio_codec_type = cc.audio_codec_type;
    config.audio_bit_rate = cc.audio_bit_rate;
    if (cc.enable_data_channel_signaling) {
      config.data_channel_signaling = cc.data_channel_signaling;
    }
    config.data_channel_signaling_timeout = cc.data_channel_signaling_timeout;
    if (cc.enable_ignore_disconnect_websocket) {
      config.ignore_disconnect_websocket = cc.ignore_disconnect_websocket;
    }
    config.disconnect_wait_timeout = cc.disconnect_wait_timeout;
    config.data_channel_messaging = cc.data_channel_messaging;
    if (!cc.metadata.empty()) {
      boost::json::error_code ec;
      auto md = boost::json::parse(cc.metadata, ec);
      if (ec) {
        RTC_LOG(LS_WARNING) << "Invalid JSON: metadata=" << cc.metadata;
      } else {
        config.metadata = md;
      }
    }

    ioc_.reset(new boost::asio::io_context(1));
    auto on_notify = [this](std::string json) {
      std::lock_guard<std::mutex> guard(event_mutex_);
      event_queue_.push_back([this, json = std::move(json)]() {
        // ここは Unity スレッドから呼ばれる
        if (on_notify_) {
          on_notify_(std::move(json));
        }
      });
    };
    auto on_push = [this](std::string json) {
      std::lock_guard<std::mutex> guard(event_mutex_);
      event_queue_.push_back([this, json = std::move(json)]() {
        // ここは Unity スレッドから呼ばれる
        if (on_push_) {
          on_push_(std::move(json));
        }
      });
    };
    auto on_message = [this](std::string label, std::string data) {
      std::lock_guard<std::mutex> guard(event_mutex_);
      event_queue_.push_back(
          [this, label = std::move(label), data = std::move(data)]() {
            // ここは Unity スレッドから呼ばれる
            if (on_message_) {
              on_message_(std::move(label), std::move(data));
            }
          });
    };
    config.on_notify = on_notify;
    config.on_push = on_push;
    config.on_message = on_message;
    config.on_disconnect = on_disconnect;
    signaling_ =
        SoraSignaling::Create(*ioc_, rtc_manager_.get(), std::move(config));
    signaling_->Connect();
  }

  thread_ = rtc::Thread::Create();
  if (!thread_->SetName("Sora IO Thread", nullptr)) {
    RTC_LOG(LS_INFO) << "Failed to set thread name";
    on_disconnect((int)sora_conf::ErrorCode::INTERNAL_ERROR,
                  "Failed to set thread name");
    return;
  }
  if (!thread_->Start()) {
    RTC_LOG(LS_INFO) << "Failed to start thread";
    on_disconnect((int)sora_conf::ErrorCode::INTERNAL_ERROR,
                  "Failed to start thread");
    return;
  }
  thread_->PostTask(RTC_FROM_HERE, [this]() {
    auto guard = boost::asio::make_work_guard(*ioc_);
    RTC_LOG(LS_INFO) << "io_context started";
    ioc_->run();
    RTC_LOG(LS_INFO) << "io_context finished";
  });
}
void Sora::Close() {
  if (signaling_ == nullptr) {
    return;
  }
  signaling_->Close();
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
  if (capturer_ != nullptr && capturer_type_ != 0) {
    static_cast<UnityCameraCapturer*>(capturer_.get())->OnRender();
  }
}
void Sora::ProcessAudio(const void* p, int offset, int samples) {
  if (!unity_adm_) {
    return;
  }
  // 今のところステレオデータを渡すようにしてるので2倍する
  unity_adm_->ProcessAudioData((const float*)p + offset, samples * 2);
}
void Sora::SetOnHandleAudio(std::function<void(const int16_t*, int, int)> f) {
  on_handle_audio_ = f;
}

rtc::scoped_refptr<UnityAudioDevice> Sora::CreateADM(
    webrtc::TaskQueueFactory* task_queue_factory,
    bool dummy_audio,
    bool unity_audio_input,
    bool unity_audio_output,
    std::function<void(const int16_t*, int, int)> on_handle_audio,
    std::string audio_recording_device,
    std::string audio_playout_device,
    rtc::Thread* worker_thread) {
  rtc::scoped_refptr<webrtc::AudioDeviceModule> adm;

  if (dummy_audio) {
    adm = worker_thread->Invoke<rtc::scoped_refptr<webrtc::AudioDeviceModule> >(
        RTC_FROM_HERE, [&] {
          return webrtc::AudioDeviceModule::Create(
              webrtc::AudioDeviceModule::kDummyAudio, task_queue_factory);
        });
  } else {
#if defined(SORA_UNITY_SDK_WINDOWS)
    adm = worker_thread->Invoke<rtc::scoped_refptr<webrtc::AudioDeviceModule> >(
        RTC_FROM_HERE, [&] {
          return webrtc::CreateWindowsCoreAudioAudioDeviceModule(
              task_queue_factory);
        });
#elif defined(SORA_UNITY_SDK_ANDROID)
    adm = worker_thread->Invoke<rtc::scoped_refptr<webrtc::AudioDeviceModule> >(
        RTC_FROM_HERE, [] {
          JNIEnv* env = webrtc::AttachCurrentThreadIfNeeded();
          auto context = GetAndroidApplicationContext(env);
          return webrtc::CreateJavaAudioDeviceModule(env, context.obj());
        });
#else
    adm = worker_thread->Invoke<rtc::scoped_refptr<webrtc::AudioDeviceModule> >(
        RTC_FROM_HERE, [&] {
          return webrtc::AudioDeviceModule::Create(
              webrtc::AudioDeviceModule::kPlatformDefaultAudio,
              task_queue_factory);
        });
#endif
  }

  return worker_thread->Invoke<rtc::scoped_refptr<UnityAudioDevice> >(
      RTC_FROM_HERE, [&] {
        return UnityAudioDevice::Create(adm, !unity_audio_input,
                                        !unity_audio_output, on_handle_audio,
                                        task_queue_factory);
      });
}

rtc::scoped_refptr<rtc::AdaptedVideoTrackSource> Sora::CreateVideoCapturer(
    int capturer_type,
    void* unity_camera_texture,
    std::string video_capturer_device,
    int video_width,
    int video_height,
    rtc::Thread* signaling_thread) {
  if (capturer_type == 0) {
    // 実カメラ（デバイス）を使う
    // TODO(melpon): framerate をちゃんと設定する
#if defined(SORA_UNITY_SDK_MACOS) || defined(SORA_UNITY_SDK_IOS)
    return MacCapturer::Create(video_width, video_height, 30,
                               video_capturer_device);
#elif defined(SORA_UNITY_SDK_ANDROID)
    JNIEnv* jni = webrtc::AttachCurrentThreadIfNeeded();
    return AndroidCapturer::Create(jni, signaling_thread, video_width,
                                   video_height, 30, video_capturer_device);
#else
    return DeviceVideoCapturer::Create(video_width, video_height, 30,
                                       video_capturer_device);
#endif
  } else {
    // Unity のカメラからの映像を使う
    return UnityCameraCapturer::Create(&UnityContext::Instance(),
                                       unity_camera_texture, video_width,
                                       video_height);
  }
}

void Sora::GetStats(std::function<void(std::string)> on_get_stats) {
  auto conn = signaling_ == nullptr ? nullptr : signaling_->GetRTCConnection();
  if (signaling_ == nullptr || conn == nullptr) {
    std::lock_guard<std::mutex> guard(event_mutex_);
    event_queue_.push_back([on_get_stats = std::move(on_get_stats)]() {
      // ここは Unity スレッドから呼ばれる
      on_get_stats("[]");
    });
    return;
  }

  conn->GetStats(
      [this, on_get_stats = std::move(on_get_stats)](
          const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report) {
        std::string json = report->ToJson();
        std::lock_guard<std::mutex> guard(event_mutex_);
        event_queue_.push_back(
            [on_get_stats = std::move(on_get_stats), json = std::move(json)]() {
              // ここは Unity スレッドから呼ばれる
              on_get_stats(std::move(json));
            });
      });
}

void Sora::SendMessage(const std::string& label, const std::string& data) {
  if (signaling_ == nullptr) {
    return;
  }
  signaling_->SendMessage(label, data);
}

}  // namespace sora
