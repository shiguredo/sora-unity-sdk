#include "sora.h"

#include "api/task_queue/default_task_queue_factory.h"
#include "modules/audio_device/include/audio_device_factory.h"

#ifdef SORA_UNITY_SDK_ANDROID
#include "sdk/android/native_api/audio_device_module/audio_device_android.h"
#include "sdk/android/native_api/jni/jvm.h"
#endif

#ifdef SORA_UNITY_SDK_ANDROID
#include "android_capturer.h"
#endif

namespace sora {

Sora::Sora(UnityContext* context) : ioc_(1), context_(context) {
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

bool Sora::Connect(std::string unity_version,
                   std::string signaling_url,
                   std::string channel_id,
                   std::string metadata,
                   std::string role,
                   bool multistream,
                   int capturer_type,
                   void* unity_camera_texture,
                   std::string video_capturer_device,
                   int video_width,
                   int video_height,
                   std::string video_codec,
                   int video_bitrate,
                   bool unity_audio_input,
                   bool unity_audio_output,
                   std::string audio_recording_device,
                   std::string audio_playout_device,
                   std::string audio_codec,
                   int audio_bitrate) {
  signaling_url_ = std::move(signaling_url);
  channel_id_ = std::move(channel_id);

  RTC_LOG(LS_INFO) << "Sora::Connect unity_version=" << unity_version
                   << " signaling_url =" << signaling_url_
                   << " channel_id=" << channel_id_ << " metadata=" << metadata
                   << " role=" << role << " multistream=" << multistream
                   << " capturer_type=" << capturer_type
                   << " unity_camera_texture=0x" << unity_camera_texture
                   << " video_capturer_device=" << video_capturer_device
                   << " video_width=" << video_width
                   << " video_height=" << video_height
                   << " unity_audio_input=" << unity_audio_input
                   << " unity_audio_output=" << unity_audio_output
                   << " audio_recording_device=" << audio_recording_device
                   << " audio_playout_device=" << audio_playout_device;

  if (role != "upstream" && role != "downstream" && role != "sendonly" &&
      role != "recvonly" && role != "sendrecv") {
    RTC_LOG(LS_ERROR) << "Invalid role: " << role;
    return false;
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

  auto task_queue_factory = webrtc::CreateDefaultTaskQueueFactory();
  unity_adm_ = CreateADM(task_queue_factory.get(), false, unity_audio_input,
                         unity_audio_output, on_handle_audio_,
                         audio_recording_device, audio_playout_device);
  if (!unity_adm_) {
    return false;
  }

  std::unique_ptr<rtc::Thread> signaling_thread = rtc::Thread::Create();

  RTCManagerConfig config;
  config.audio_recording_device = audio_recording_device;
  config.audio_playout_device = audio_playout_device;

  if (role == "upstream" || role == "sendonly" || role == "sendrecv") {
    // 送信側は capturer を設定する。送信のみの場合は playout の設定はしない
    rtc::scoped_refptr<rtc::AdaptedVideoTrackSource> capturer =
        CreateVideoCapturer(capturer_type, unity_camera_texture,
                            video_capturer_device, video_width, video_height,
                            signaling_thread.get());
    if (!capturer) {
      return false;
    }

    capturer_ = capturer;
    capturer_type_ = capturer_type;

    config.no_playout = role == "upstream" || role == "sendonly";

    config.audio_recording_device = audio_recording_device;
    config.audio_playout_device = audio_playout_device;
    rtc_manager_ = RTCManager::Create(
        config, std::move(capturer), renderer_.get(), unity_adm_,
        std::move(task_queue_factory), std::move(signaling_thread));

  } else {
    // 受信側は capturer を作らず、video, recording の設定もしない
    RTCManagerConfig config;
    config.no_recording = true;
    config.no_video = true;

    config.audio_recording_device = audio_recording_device;
    config.audio_playout_device = audio_playout_device;
    rtc_manager_ = RTCManager::Create(config, nullptr, renderer_.get(),
                                      unity_adm_, std::move(task_queue_factory),
                                      std::move(signaling_thread));
  }

  {
    RTC_LOG(LS_INFO) << "Start Signaling: url=" << signaling_url_
                     << " channel_id=" << channel_id_;
    SoraSignalingConfig config;
    config.unity_version = unity_version;
    config.role = role == "upstream"
                      ? SoraSignalingConfig::Role::Upstream
                      : role == "downstream"
                            ? SoraSignalingConfig::Role::Downstream
                            : role == "sendonly"
                                  ? SoraSignalingConfig::Role::Sendonly
                                  : role == "recvonly"
                                        ? SoraSignalingConfig::Role::Recvonly
                                        : SoraSignalingConfig::Role::Sendrecv;
    config.multistream = multistream;
    config.signaling_url = signaling_url_;
    config.channel_id = channel_id_;
    config.video_codec = video_codec;
    config.video_bitrate = video_bitrate;
    config.audio_codec = audio_codec;
    config.audio_bitrate = audio_bitrate;
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
    std::string audio_playout_device) {
  rtc::scoped_refptr<webrtc::AudioDeviceModule> adm;

  if (dummy_audio) {
    adm = webrtc::AudioDeviceModule::Create(
        webrtc::AudioDeviceModule::kDummyAudio, task_queue_factory);
  } else {
#if defined(SORA_UNITY_SDK_WINDOWS)
    adm = webrtc::CreateWindowsCoreAudioAudioDeviceModule(task_queue_factory);
#elif defined(SORA_UNITY_SDK_ANDROID)
    // Context context = UnityPlayer.currentActivity.getApplicationContext()
    // を頑張って C++ から呼んでるだけ
    JNIEnv* env = webrtc::AttachCurrentThreadIfNeeded();
    webrtc::ScopedJavaLocalRef<jclass> upcls =
        webrtc::GetClass(env, "com/unity3d/player/UnityPlayer");
    jfieldID actid = env->GetStaticFieldID(upcls.obj(), "currentActivity",
                                           "Landroid/app/Activity;");
    jobject activity = env->GetStaticObjectField(upcls.obj(), actid);

    jclass actcls = env->GetObjectClass(activity);
    jmethodID ctxid = env->GetMethodID(actcls, "getApplicationContext",
                                       "()Landroid/content/Context;");
    jobject context = env->CallObjectMethod(activity, ctxid);

    adm = webrtc::CreateJavaAudioDeviceModule(env, context);
#else
    adm = webrtc::AudioDeviceModule::Create(
        webrtc::AudioDeviceModule::kPlatformDefaultAudio, task_queue_factory);
#endif
  }

  return UnityAudioDevice::Create(adm, !unity_audio_input, !unity_audio_output,
                                  on_handle_audio, task_queue_factory);
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
#if defined(SORA_UNITY_SDK_MACOS)
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

}  // namespace sora
