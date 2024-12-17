#include "sora.h"
#include "sora_version.h"

// WebRTC
#include <api/audio_codecs/builtin_audio_decoder_factory.h>
#include <api/audio_codecs/builtin_audio_encoder_factory.h>
#include <api/create_peerconnection_factory.h>
#include <api/rtc_event_log/rtc_event_log_factory.h>
#include <api/task_queue/default_task_queue_factory.h>
#include <media/engine/webrtc_media_engine.h>
#include <modules/audio_device/include/audio_device.h>
#include <modules/audio_device/include/audio_device_factory.h>
#include <modules/audio_processing/include/audio_processing.h>
#include <modules/video_capture/video_capture.h>
#include <modules/video_capture/video_capture_factory.h>
#include <pc/video_track_source_proxy.h>
#include <rtc_base/crypto_random.h>
#include <rtc_base/logging.h>
#include <rtc_base/ssl_adapter.h>

// Sora
#include <sora/audio_device_module.h>
#include <sora/camera_device_capturer.h>
#include <sora/java_context.h>
#include <sora/rtc_stats.h>
#include <sora/sora_peer_connection_factory.h>
#include <sora/sora_video_decoder_factory.h>
#include <sora/sora_video_encoder_factory.h>

#ifdef SORA_UNITY_SDK_ANDROID
#include <sora/android/android_capturer.h>
#include "android_helper/android_context.h"
#endif

#ifdef SORA_UNITY_SDK_IOS
#include <sora/mac/mac_capturer.h>
#include "mac_helper/ios_audio_init.h"
#endif

#ifdef SORA_UNITY_SDK_MACOS
#include <sora/mac/mac_capturer.h>
#endif

namespace sora_unity_sdk {

Sora::Sora(UnityContext* context) : unity_context_(context) {
  ptrid_ = IdPointer::Instance().Register(this);
#if defined(SORA_UNITY_SDK_ANDROID)
  auto env = sora::GetJNIEnv();
  android_context_ =
      ::sora_unity_sdk::GetAndroidApplicationContext((JNIEnv*)env);
#endif
}

Sora::~Sora() {
  RTC_LOG(LS_INFO) << "Sora object destroy started";

  IdPointer::Instance().Unregister(ptrid_);

  renderer_.reset();
#if defined(SORA_UNITY_SDK_ANDROID)
  if (capturer_ != nullptr && capturer_type_ == 0) {
    static_cast<sora::AndroidCapturer*>(capturer_.get())->Stop();
  }
#endif
#if defined(SORA_UNITY_SDK_IOS) || defined(SORA_UNITY_SDK_MACOS)
  if (capturer_ != nullptr && capturer_type_ == 0) {
    static_cast<sora::MacCapturer*>(capturer_.get())->Stop();
  }
#endif
  if (capturer_ != nullptr && capturer_type_ != 0) {
    static_cast<UnityCameraCapturer*>(capturer_.get())->Stop();
  }
  capturer_sink_ = nullptr;
  capturer_ = nullptr;
  unity_adm_ = nullptr;

  video_sender_ = nullptr;
  audio_track_ = nullptr;
  video_track_ = nullptr;

  if (ioc_ != nullptr) {
    ioc_->stop();
  }
  signaling_.reset();
  ioc_.reset();
  if (io_thread_) {
    io_thread_->Stop();
    io_thread_.reset();
  }
  sora_context_ = nullptr;
  RTC_LOG(LS_INFO) << "Sora object destroy finished";
}

#define SORA_CLIENT \
  "Sora Unity SDK " SORA_UNITY_SDK_VERSION " (" SORA_UNITY_SDK_COMMIT_SHORT ")"

void Sora::SetOnAddTrack(
    std::function<void(ptrid_t, std::string)> on_add_track) {
  on_add_track_ = on_add_track;
}
void Sora::SetOnRemoveTrack(
    std::function<void(ptrid_t, std::string)> on_remove_track) {
  on_remove_track_ = on_remove_track;
}
void Sora::SetOnSetOffer(std::function<void(std::string)> on_set_offer) {
  on_set_offer_ = std::move(on_set_offer);
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
void Sora::SetOnDataChannel(std::function<void(std::string)> on_data_channel) {
  on_data_channel_ = std::move(on_data_channel);
}
void Sora::SetOnCapturerFrame(
    std::function<void(std::string)> on_capturer_frame) {
  on_capturer_frame_ = std::move(on_capturer_frame);
}

void Sora::DispatchEvents() {
  auto self = shared_from_this();
  while (true) {
    std::function<void()> f;
    {
      std::lock_guard<std::mutex> guard(event_mutex_);
      if (event_queue_.empty()) {
        break;
      }
      f = std::move(event_queue_.front());
      event_queue_.pop_front();
    }
    f();
  }
}

static sora_conf::VideoFrame VideoFrameToConfig(
    const webrtc::VideoFrame& frame) {
  sora_conf::VideoFrame f;
  f.baseptr = reinterpret_cast<int64_t>(&frame);
  f.id = frame.id();
  f.timestamp_us = frame.timestamp_us();
  f.timestamp = frame.rtp_timestamp();
  f.ntp_time_ms = frame.ntp_time_ms();
  f.rotation = (int)frame.rotation();
  auto& v = f.video_frame_buffer;
  auto vfb = frame.video_frame_buffer();
  v.baseptr = reinterpret_cast<int64_t>(vfb.get());
  v.type = (sora_conf::VideoFrameBuffer::Type)vfb->type();
  v.width = vfb->width();
  v.height = vfb->height();
  if (vfb->type() == webrtc::VideoFrameBuffer::Type::kI420) {
    auto p = vfb->GetI420();
    v.i420_stride_y = p->StrideY();
    v.i420_stride_u = p->StrideU();
    v.i420_stride_v = p->StrideV();
    v.i420_data_y = reinterpret_cast<int64_t>(p->DataY());
    v.i420_data_u = reinterpret_cast<int64_t>(p->DataU());
    v.i420_data_v = reinterpret_cast<int64_t>(p->DataV());
  }
  if (vfb->type() == webrtc::VideoFrameBuffer::Type::kNV12) {
    auto p = vfb->GetNV12();
    v.nv12_stride_y = p->StrideY();
    v.nv12_stride_uv = p->StrideUV();
    v.nv12_data_y = reinterpret_cast<int64_t>(p->DataY());
    v.nv12_data_uv = reinterpret_cast<int64_t>(p->DataUV());
  }
  return f;
}

void Sora::Connect(const sora_conf::internal::ConnectConfig& cc) {
  auto on_disconnect = [this](int error_code, std::string reason) {
    PushEvent([this, error_code, reason = std::move(reason)]() {
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

  static bool ios_audio_initializing = false;
  if (!ios_audio_initializing) {
    ios_audio_initializing = true;
    IosAudioInit(
        [this, on_disconnect = std::move(on_disconnect)](std::string error) {
          if (!error.empty()) {
            RTC_LOG(LS_ERROR) << "Failed to IosAudioInit: error=" << error;
            on_disconnect((int)sora_conf::ErrorCode::INTERNAL_ERROR,
                          "Failed to IosAudioInit: error=" + error);
          }
          ios_audio_initializing = false;
        });
  }
  DoConnect(cc, std::move(on_disconnect));
#else
  DoConnect(cc, std::move(on_disconnect));
#endif
}

void Sora::DoConnect(const sora_conf::internal::ConnectConfig& cc,
                     std::function<void(int, std::string)> on_disconnect) {
  RTC_LOG(LS_INFO) << "Sora::Connect " << jsonif::to_json(cc);

  if (cc.role != "sendonly" && cc.role != "recvonly" && cc.role != "sendrecv") {
    RTC_LOG(LS_ERROR) << "Invalid role: " << cc.role;
    on_disconnect((int)sora_conf::ErrorCode::INVALID_PARAMETER,
                  "Invalid role: " + cc.role);
    return;
  }
  if (cc.signaling_url.empty()) {
    RTC_LOG(LS_ERROR) << "Signaling URL is empty";
    on_disconnect((int)sora_conf::ErrorCode::INVALID_PARAMETER,
                  "Signaling URL is empty");
    return;
  }

  // このスレッド上の env と context
  void* env = sora::GetJNIEnv();
  void* android_context = GetAndroidApplicationContext(env);

  sora::SoraClientContextConfig client_config;
  client_config.use_audio_device = false;
  if (cc.has_use_hardware_encoder()) {
    client_config.use_hardware_encoder = cc.use_hardware_encoder;
  }
  client_config.configure_dependencies =
      [&, this](webrtc::PeerConnectionFactoryDependencies& dependencies) {
        // worker 上の env
        auto worker_env = dependencies.worker_thread->BlockingCall(
            [] { return sora::GetJNIEnv(); });
    // worker 上の context
#if defined(SORA_UNITY_SDK_ANDROID)
        void* worker_context =
            dependencies.worker_thread->BlockingCall([worker_env] {
              return ::sora_unity_sdk::GetAndroidApplicationContext(
                         (JNIEnv*)worker_env)
                  .Release();
            });
#else
        void* worker_context = nullptr;
#endif

        unity_adm_ = CreateADM(
            dependencies.task_queue_factory.get(), cc.no_audio_device,
            cc.unity_audio_input, cc.unity_audio_output, on_handle_audio_,
            cc.audio_recording_device, cc.audio_playout_device,
            dependencies.worker_thread, worker_env, worker_context);
        dependencies.worker_thread->BlockingCall(
            [&] { dependencies.adm = unity_adm_; });

#if defined(SORA_UNITY_SDK_ANDROID)
        dependencies.worker_thread->BlockingCall([worker_env, worker_context] {
          ((JNIEnv*)worker_env)->DeleteLocalRef((jobject)worker_context);
        });
#endif
      };

  sora_context_ = sora::SoraClientContext::Create(client_config);

  if (sora_context_ == nullptr) {
    RTC_LOG(LS_ERROR) << "Failed to create PeerConnectionFactory";
    on_disconnect((int)sora_conf::ErrorCode::INTERNAL_ERROR,
                  "Failed to create PeerConnectionFactory");
    return;
  }

  if (!InitADM(unity_adm_, cc.audio_recording_device,
               cc.audio_playout_device)) {
    on_disconnect((int)sora_conf::ErrorCode::INTERNAL_ERROR,
                  "Failed to InitADM");
    return;
  }

  renderer_.reset(new UnityRenderer());

  if (cc.role == "sendonly" || cc.role == "sendrecv") {
    std::function<void(const webrtc::VideoFrame& frame)> on_frame;
    if (on_capturer_frame_) {
      on_frame = [on_frame =
                      on_capturer_frame_](const webrtc::VideoFrame& frame) {
        sora_conf::VideoFrame f = VideoFrameToConfig(frame);
        on_frame(jsonif::to_json(f));
      };
    }

    auto capturer = CreateVideoCapturer(
        cc.camera_config.capturer_type,
        (void*)cc.camera_config.unity_camera_texture, cc.no_video_device,
        cc.camera_config.video_capturer_device, cc.camera_config.video_width,
        cc.camera_config.video_height, cc.camera_config.video_fps, on_frame,
        sora_context_->signaling_thread(), env, android_context,
        unity_context_);
    if (!cc.no_video_device && !capturer) {
      on_disconnect((int)sora_conf::ErrorCode::INTERNAL_ERROR,
                    "Capturer Init Failed");
      return;
    }

    capturer_ = capturer;
    capturer_type_ = cc.camera_config.capturer_type;

    std::string audio_track_id = rtc::CreateRandomString(16);
    audio_track_ = sora_context_->peer_connection_factory()->CreateAudioTrack(
        audio_track_id, sora_context_->peer_connection_factory()
                            ->CreateAudioSource(cricket::AudioOptions())
                            .get());

    if (capturer) {
      std::string video_track_id = rtc::CreateRandomString(16);
      video_track_ = sora_context_->peer_connection_factory()->CreateVideoTrack(
          capturer, video_track_id);
    }
  }

  {
    RTC_LOG(LS_INFO) << "Start Signaling: cc=" << jsonif::to_json(cc);
    ioc_.reset(new boost::asio::io_context(1));
    sora::SoraSignalingConfig config;
    config.observer = shared_from_this();
    config.pc_factory = sora_context_->peer_connection_factory();
    config.io_context = ioc_.get();
    config.role = cc.role;
    config.sora_client = SORA_CLIENT;
    if (cc.has_multistream()) {
      config.multistream = cc.multistream;
    }
    if (cc.has_spotlight()) {
      config.spotlight = cc.spotlight;
    }
    config.spotlight_number = cc.spotlight_number;
    config.spotlight_focus_rid = cc.spotlight_focus_rid;
    config.spotlight_unfocus_rid = cc.spotlight_unfocus_rid;
    if (cc.has_simulcast()) {
      config.simulcast = cc.simulcast;
    }
    config.simulcast_rid = cc.simulcast_rid;
    config.signaling_urls = cc.signaling_url;
    config.channel_id = cc.channel_id;
    config.client_id = cc.client_id;
    config.bundle_id = cc.bundle_id;
    config.video = cc.video;
    config.audio = cc.audio;
    config.video_codec_type = cc.video_codec_type;
    if (!cc.video_vp9_params.empty()) {
      boost::system::error_code ec;
      auto md = boost::json::parse(cc.video_vp9_params, ec);
      if (ec) {
        RTC_LOG(LS_WARNING)
            << "Invalid JSON: video_vp9_params=" << cc.video_vp9_params;
      } else {
        config.video_vp9_params = md;
      }
    }
    if (!cc.video_av1_params.empty()) {
      boost::system::error_code ec;
      auto md = boost::json::parse(cc.video_av1_params, ec);
      if (ec) {
        RTC_LOG(LS_WARNING)
            << "Invalid JSON: video_av1_params=" << cc.video_av1_params;
      } else {
        config.video_av1_params = md;
      }
    }
    if (!cc.video_h264_params.empty()) {
      boost::system::error_code ec;
      auto md = boost::json::parse(cc.video_h264_params, ec);
      if (ec) {
        RTC_LOG(LS_WARNING)
            << "Invalid JSON: video_h264_params=" << cc.video_h264_params;
      } else {
        config.video_h264_params = md;
      }
    }
    config.video_bit_rate = cc.video_bit_rate;
    config.audio_codec_type = cc.audio_codec_type;
    config.audio_bit_rate = cc.audio_bit_rate;
    if (cc.has_data_channel_signaling()) {
      config.data_channel_signaling = cc.data_channel_signaling;
    }
    config.data_channel_signaling_timeout = cc.data_channel_signaling_timeout;
    if (cc.has_ignore_disconnect_websocket()) {
      config.ignore_disconnect_websocket = cc.ignore_disconnect_websocket;
    }
    config.disconnect_wait_timeout = cc.disconnect_wait_timeout;
    for (const auto& dc : cc.data_channels) {
      sora::SoraSignalingConfig::DataChannel d;
      d.label = dc.label;
      d.direction = dc.direction;
      if (dc.has_ordered()) {
        d.ordered = dc.ordered;
      }
      if (dc.has_max_packet_life_time()) {
        d.max_packet_life_time = dc.max_packet_life_time;
      }
      if (dc.has_max_retransmits()) {
        d.max_retransmits = dc.max_retransmits;
      }
      if (dc.has_protocol()) {
        d.protocol = dc.protocol;
      }
      if (dc.has_compress()) {
        d.compress = dc.compress;
      }
      config.data_channels.push_back(std::move(d));
    }
    if (!cc.metadata.empty()) {
      boost::system::error_code ec;
      auto md = boost::json::parse(cc.metadata, ec);
      if (ec) {
        RTC_LOG(LS_WARNING) << "Invalid JSON: metadata=" << cc.metadata;
      } else {
        config.metadata = md;
      }
    }
    if (!cc.signaling_notify_metadata.empty()) {
      boost::system::error_code ec;
      auto md = boost::json::parse(cc.signaling_notify_metadata, ec);
      if (ec) {
        RTC_LOG(LS_WARNING) << "Invalid JSON: signaling_notify_metadata="
                            << cc.signaling_notify_metadata;
      } else {
        config.signaling_notify_metadata = md;
      }
    }
    config.insecure = cc.insecure;
    config.proxy_url = cc.proxy_url;
    config.proxy_username = cc.proxy_username;
    config.proxy_password = cc.proxy_password;
    config.proxy_agent = cc.proxy_agent;
    config.audio_streaming_language_code = cc.audio_streaming_language_code;
    if (cc.has_forwarding_filter()) {
      sora::SoraSignalingConfig::ForwardingFilter ff;
      if (cc.forwarding_filter.has_action()) {
        ff.action = cc.forwarding_filter.action;
      }
      for (const auto& rs : cc.forwarding_filter.rules) {
        std::vector<sora::SoraSignalingConfig::ForwardingFilter::Rule> ffrs;
        for (const auto& r : rs.rules) {
          sora::SoraSignalingConfig::ForwardingFilter::Rule ffr;
          ffr.field = r.field;
          ffr.op = r.op;
          ffr.values = r.values;
          ffrs.push_back(ffr);
        }
        ff.rules.push_back(ffrs);
      }
      if (cc.forwarding_filter.has_version()) {
        ff.version = cc.forwarding_filter.version;
      }
      if (cc.forwarding_filter.has_metadata()) {
        boost::system::error_code ec;
        auto ffmd = boost::json::parse(cc.forwarding_filter.metadata, ec);
        if (ec) {
          RTC_LOG(LS_WARNING) << "Invalid JSON: forwarding_filter metadata="
                              << cc.forwarding_filter.metadata;
        } else {
          ff.metadata = ffmd;
        }
      }
      config.forwarding_filter = ff;
    }
    if (cc.has_client_cert()) {
      config.client_cert = cc.client_cert;
    }
    if (cc.has_client_key()) {
      config.client_key = cc.client_key;
    }
    if (cc.has_ca_cert()) {
      config.ca_cert = cc.ca_cert;
    }
    config.network_manager =
        sora_context_->signaling_thread()->BlockingCall([this]() {
          return sora_context_->connection_context()->default_network_manager();
        });
    config.socket_factory =
        sora_context_->signaling_thread()->BlockingCall([this]() {
          return sora_context_->connection_context()->default_socket_factory();
        });
    config.user_agent = std::optional<std::string>(
        "Mozilla/5.0 (Sora Unity SDK/" SORA_UNITY_SDK_VERSION ")");

    signaling_ = sora::SoraSignaling::Create(std::move(config));
    signaling_->Connect();
  }

  io_thread_ = rtc::Thread::Create();
  if (!io_thread_->SetName("Sora IO Thread", nullptr)) {
    RTC_LOG(LS_INFO) << "Failed to set thread name";
    on_disconnect((int)sora_conf::ErrorCode::INTERNAL_ERROR,
                  "Failed to set thread name");
    return;
  }
  if (!io_thread_->Start()) {
    RTC_LOG(LS_INFO) << "Failed to start thread";
    on_disconnect((int)sora_conf::ErrorCode::INTERNAL_ERROR,
                  "Failed to start thread");
    return;
  }
  io_thread_->PostTask([this]() {
    auto guard = boost::asio::make_work_guard(*ioc_);
    RTC_LOG(LS_INFO) << "io_context started";
    ioc_->run();
    RTC_LOG(LS_INFO) << "io_context finished";
  });
}

void Sora::Disconnect() {
  if (signaling_ == nullptr) {
    return;
  }
  signaling_->Disconnect();
}

void Sora::SwitchCamera(const sora_conf::internal::CameraConfig& cc) {
  if (!set_offer_) {
    return;
  }

  RTC_LOG(LS_INFO) << "SwitchCamera: " << jsonif::to_json(cc);

  // このあたりのキャプチャラ作成は、IO スレッドではなく Unity スレッドで行う。
  // （DoConnect で作成したキャプチャラは Unity スレッド上で実行されてるので、
  //   全て同じスレッドでやる必要がある）
  std::function<void(const webrtc::VideoFrame& frame)> on_frame;
  if (on_capturer_frame_) {
    on_frame = [on_frame =
                    on_capturer_frame_](const webrtc::VideoFrame& frame) {
      sora_conf::VideoFrame f = VideoFrameToConfig(frame);
      on_frame(jsonif::to_json(f));
    };
  }

  void* env = sora::GetJNIEnv();
  void* android_context = GetAndroidApplicationContext(env);

#if defined(SORA_UNITY_SDK_ANDROID)
  if (capturer_ != nullptr && capturer_type_ == 0) {
    static_cast<sora::AndroidCapturer*>(capturer_.get())->Stop();
  }
#endif
#if defined(SORA_UNITY_SDK_IOS) || defined(SORA_UNITY_SDK_MACOS)
  if (capturer_ != nullptr && capturer_type_ == 0) {
    static_cast<sora::MacCapturer*>(capturer_.get())->Stop();
  }
#endif
  if (capturer_ != nullptr && capturer_type_ != 0) {
    static_cast<UnityCameraCapturer*>(capturer_.get())->Stop();
  }
  capturer_ = nullptr;

  auto capturer = CreateVideoCapturer(
      cc.capturer_type, (void*)cc.unity_camera_texture, false,
      cc.video_capturer_device, cc.video_width, cc.video_height, cc.video_fps,
      on_frame, sora_context_->signaling_thread(), env, android_context,
      unity_context_);
  if (!capturer) {
    RTC_LOG(LS_ERROR) << "Failed to CreateVideoCapturer";
    return;
  }

  capturer_ = capturer;
  capturer_type_ = cc.capturer_type;

  std::string video_track_id = rtc::CreateRandomString(16);
  auto video_track = sora_context_->peer_connection_factory()->CreateVideoTrack(
      capturer, video_track_id);

  // video_track_ をこのスレッド(Unity スレッド)で設定したいので、
  // IO スレッドの実行完了を待つ
  std::promise<void> p;
  std::future<void> f = p.get_future();
  boost::asio::post(*ioc_,
                    [self = shared_from_this(), cc = cc, video_track, &p]() {
                      self->DoSwitchCamera(cc, video_track);
                      p.set_value();
                    });
  f.wait();

  video_track_ = video_track;
}
void Sora::DoSwitchCamera(
    const sora_conf::internal::CameraConfig& cc,
    rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track) {
  if (video_track_ == nullptr) {
    webrtc::RTCErrorOr<rtc::scoped_refptr<webrtc::RtpSenderInterface>>
        video_result = signaling_->GetPeerConnection()->AddTrack(video_track,
                                                                 {stream_id_});
    video_sender_ = video_result.value();

    auto track_id = renderer_->AddTrack(video_track.get());
    PushEvent([this, track_id]() {
      if (on_add_track_) {
        on_add_track_(track_id, "");
      }
    });
  } else {
    renderer_->ReplaceTrack(video_track_.get(), video_track.get());
  }
  video_sender_->SetTrack(video_track.get());
}

void Sora::RenderCallbackStatic(int event_id) {
  auto sora = (Sora*)IdPointer::Instance().Lookup(event_id);
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
    rtc::Thread* worker_thread,
    void* jni_env,
    void* android_context) {
  rtc::scoped_refptr<webrtc::AudioDeviceModule> adm;

  if (dummy_audio) {
    adm = worker_thread->BlockingCall([&] {
      return webrtc::AudioDeviceModule::Create(
          webrtc::AudioDeviceModule::kDummyAudio, task_queue_factory);
    });
  } else {
    sora::AudioDeviceModuleConfig config;
    config.audio_layer = webrtc::AudioDeviceModule::kPlatformDefaultAudio;
    config.task_queue_factory = task_queue_factory;
    config.jni_env = jni_env;
    config.application_context = android_context;
    adm = worker_thread->BlockingCall(
        [&] { return sora::CreateAudioDeviceModule(config); });
  }

  return worker_thread->BlockingCall([&] {
    return UnityAudioDevice::Create(adm, !unity_audio_input,
                                    !unity_audio_output, on_handle_audio,
                                    task_queue_factory);
  });
}

bool Sora::InitADM(rtc::scoped_refptr<webrtc::AudioDeviceModule> adm,
                   std::string audio_recording_device,
                   std::string audio_playout_device) {
  // 録音デバイスと再生デバイスを指定する
  if (!audio_recording_device.empty()) {
    bool succeeded = false;
    int devices = adm->RecordingDevices();
    for (int i = 0; i < devices; i++) {
      char name[webrtc::kAdmMaxDeviceNameSize];
      char guid[webrtc::kAdmMaxGuidSize];
      if (adm->SetRecordingDevice(i) != 0) {
        RTC_LOG(LS_WARNING) << "Failed to SetRecordingDevice: index=" << i;
        continue;
      }
      bool available = false;
      if (adm->RecordingIsAvailable(&available) != 0) {
        RTC_LOG(LS_WARNING) << "Failed to RecordingIsAvailable: index=" << i;
        continue;
      }

      if (!available) {
        continue;
      }
      if (adm->RecordingDeviceName(i, name, guid) != 0) {
        RTC_LOG(LS_WARNING) << "Failed to RecordingDeviceName: index=" << i;
        continue;
      }
      if (audio_recording_device == name || audio_recording_device == guid) {
        RTC_LOG(LS_INFO) << "Succeeded SetRecordingDevice: index=" << i
                         << " device_name=" << name << " unique_name=" << guid;
        succeeded = true;
        break;
      }
    }
    if (!succeeded) {
      RTC_LOG(LS_ERROR) << "No recording device found: name="
                        << audio_recording_device;
      return false;
    }
  }

  if (!audio_playout_device.empty()) {
    bool succeeded = false;
    int devices = adm->PlayoutDevices();
    for (int i = 0; i < devices; i++) {
      char name[webrtc::kAdmMaxDeviceNameSize];
      char guid[webrtc::kAdmMaxGuidSize];
      if (adm->SetPlayoutDevice(i) != 0) {
        RTC_LOG(LS_WARNING) << "Failed to SetPlayoutDevice: index=" << i;
        continue;
      }
      bool available = false;
      if (adm->PlayoutIsAvailable(&available) != 0) {
        RTC_LOG(LS_WARNING) << "Failed to PlayoutIsAvailable: index=" << i;
        continue;
      }

      if (!available) {
        continue;
      }
      if (adm->PlayoutDeviceName(i, name, guid) != 0) {
        RTC_LOG(LS_WARNING) << "Failed to PlayoutDeviceName: index=" << i;
        continue;
      }
      if (audio_playout_device == name || audio_playout_device == guid) {
        RTC_LOG(LS_INFO) << "Succeeded SetPlayoutDevice: index=" << i
                         << " device_name=" << name << " unique_name=" << guid;
        succeeded = true;
        break;
      }
    }
    if (!succeeded) {
      RTC_LOG(LS_ERROR) << "No playout device found: name="
                        << audio_playout_device;
      return false;
    }
  }

  return true;
}

rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> Sora::CreateVideoCapturer(
    int capturer_type,
    void* unity_camera_texture,
    bool no_video_device,
    std::string video_capturer_device,
    int video_width,
    int video_height,
    int video_fps,
    std::function<void(const webrtc::VideoFrame& frame)> on_frame,
    rtc::Thread* signaling_thread,
    void* jni_env,
    void* android_context,
    UnityContext* unity_context) {
  if (capturer_type == 0) {
    if (no_video_device) {
      return nullptr;
    }

    // 実カメラ（デバイス）を使う
    sora::CameraDeviceCapturerConfig config;
    config.width = video_width;
    config.height = video_height;
    config.fps = video_fps;
    config.on_frame = on_frame;
    config.device_name = video_capturer_device;
    config.jni_env = jni_env;
    config.application_context = android_context;
    config.signaling_thread = signaling_thread;

    return sora::CreateCameraDeviceCapturer(config);
  } else {
    // Unity のカメラからの映像を使う
    UnityCameraCapturerConfig config;
    config.context = unity_context;
    config.unity_camera_texture = unity_camera_texture;
    config.width = video_width;
    config.height = video_height;
    return UnityCameraCapturer::Create(config);
  }
}

void Sora::GetStats(std::function<void(std::string)> on_get_stats) {
  boost::asio::post(*ioc_, [self = shared_from_this(),
                            on_get_stats = std::move(on_get_stats)]() {
    auto pc = self->signaling_ == nullptr
                  ? nullptr
                  : self->signaling_->GetPeerConnection();
    if (self->signaling_ == nullptr || pc == nullptr) {
      self->PushEvent(
          [on_get_stats = std::move(on_get_stats)]() { on_get_stats("[]"); });
      return;
    }

    pc->GetStats(sora::RTCStatsCallback::Create(
                     [self, on_get_stats = std::move(on_get_stats)](
                         const rtc::scoped_refptr<const webrtc::RTCStatsReport>&
                             report) {
                       std::string json = report->ToJson();
                       self->PushEvent([on_get_stats = std::move(on_get_stats),
                                        json = std::move(json)]() {
                         on_get_stats(std::move(json));
                       });
                     })
                     .get());
  });
}

void Sora::SendMessage(const std::string& label, const std::string& data) {
  if (signaling_ == nullptr) {
    return;
  }
  signaling_->SendDataChannel(label, data);
}

void* Sora::GetAndroidApplicationContext(void* env) {
#ifdef SORA_UNITY_SDK_ANDROID
  return android_context_.obj();
#else
  return nullptr;
#endif
}

sora_conf::ErrorCode Sora::ToErrorCode(sora::SoraSignalingErrorCode ec) {
  switch (ec) {
    case sora::SoraSignalingErrorCode::CLOSE_SUCCEEDED:
      return sora_conf::ErrorCode::CLOSE_SUCCEEDED;
    case sora::SoraSignalingErrorCode::CLOSE_FAILED:
      return sora_conf::ErrorCode::CLOSE_FAILED;
    case sora::SoraSignalingErrorCode::INTERNAL_ERROR:
      return sora_conf::ErrorCode::INTERNAL_ERROR;
    case sora::SoraSignalingErrorCode::INVALID_PARAMETER:
      return sora_conf::ErrorCode::INVALID_PARAMETER;
    case sora::SoraSignalingErrorCode::WEBSOCKET_HANDSHAKE_FAILED:
      return sora_conf::ErrorCode::WEBSOCKET_HANDSHAKE_FAILED;
    case sora::SoraSignalingErrorCode::WEBSOCKET_ONCLOSE:
      return sora_conf::ErrorCode::WEBSOCKET_ONCLOSE;
    case sora::SoraSignalingErrorCode::WEBSOCKET_ONERROR:
      return sora_conf::ErrorCode::WEBSOCKET_ONERROR;
    case sora::SoraSignalingErrorCode::PEER_CONNECTION_STATE_FAILED:
      return sora_conf::ErrorCode::PEER_CONNECTION_STATE_FAILED;
    case sora::SoraSignalingErrorCode::ICE_FAILED:
      return sora_conf::ErrorCode::ICE_FAILED;
    default:
      return sora_conf::ErrorCode::INTERNAL_ERROR;
  }
}

void Sora::OnSetOffer(std::string offer) {
  PushEvent([this, offer]() {
    if (on_set_offer_) {
      on_set_offer_(offer);
    }
  });
  stream_id_ = rtc::CreateRandomString(16);
  if (audio_track_ != nullptr) {
    webrtc::RTCErrorOr<rtc::scoped_refptr<webrtc::RtpSenderInterface>>
        audio_result = signaling_->GetPeerConnection()->AddTrack(audio_track_,
                                                                 {stream_id_});
  }
  if (video_track_ != nullptr) {
    webrtc::RTCErrorOr<rtc::scoped_refptr<webrtc::RtpSenderInterface>>
        video_result = signaling_->GetPeerConnection()->AddTrack(video_track_,
                                                                 {stream_id_});
    video_sender_ = video_result.value();
  }

  if (video_track_ != nullptr) {
    auto track_id = renderer_->AddTrack(video_track_.get());
    PushEvent([this, track_id]() {
      if (on_add_track_) {
        on_add_track_(track_id, "");
      }
    });
  }

  set_offer_ = true;
}
void Sora::OnDisconnect(sora::SoraSignalingErrorCode ec, std::string message) {
  RTC_LOG(LS_INFO) << "OnDisconnect: " << message;
  renderer_.reset();
  ioc_->stop();
  PushEvent([this, ec, message = std::move(message)]() {
    if (on_disconnect_) {
      on_disconnect_((int)ToErrorCode(ec), std::move(message));
    }
  });
}
void Sora::OnNotify(std::string text) {
  PushEvent([this, text = std::move(text)]() {
    if (on_notify_) {
      on_notify_(std::move(text));
    }
  });
}
void Sora::OnPush(std::string text) {
  PushEvent([this, text = std::move(text)]() {
    if (on_push_) {
      on_push_(std::move(text));
    }
  });
}
void Sora::OnMessage(std::string label, std::string data) {
  PushEvent([this, label = std::move(label), data = std::move(data)]() {
    if (on_message_) {
      on_message_(std::move(label), std::move(data));
    }
  });
}
void Sora::OnTrack(
    rtc::scoped_refptr<webrtc::RtpTransceiverInterface> transceiver) {
  auto track = transceiver->receiver()->track();
  if (track->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
    auto connection_id = transceiver->receiver()->stream_ids()[0];
    auto track_id = renderer_->AddTrack(
        static_cast<webrtc::VideoTrackInterface*>(track.get()));
    connection_ids_.insert(std::make_pair(track_id, connection_id));
    PushEvent([this, track_id, connection_id]() {
      if (on_add_track_) {
        on_add_track_(track_id, connection_id);
      }
    });
  }
}
void Sora::OnRemoveTrack(
    rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) {
  auto track = receiver->track();
  if (track->kind() == webrtc::MediaStreamTrackInterface::kVideoKind) {
    auto track_id = renderer_->RemoveTrack(
        static_cast<webrtc::VideoTrackInterface*>(track.get()));
    auto connection_id = connection_ids_[track_id];
    connection_ids_.erase(track_id);

    if (track_id != 0) {
      PushEvent([this, track_id, connection_id]() {
        if (on_remove_track_) {
          on_remove_track_(track_id, connection_id);
        }
      });
    }
  }
}

void Sora::OnDataChannel(std::string label) {
  PushEvent([this, label]() {
    if (on_data_channel_) {
      on_data_channel_(label);
    }
  });
}

void Sora::PushEvent(std::function<void()> f) {
  std::lock_guard<std::mutex> guard(event_mutex_);
  event_queue_.push_back(std::move(f));
}

bool Sora::GetAudioEnabled() const {
  if (audio_track_ == nullptr) {
    return false;
  }
  return audio_track_->enabled();
}

void Sora::SetAudioEnabled(bool enabled) {
  if (audio_track_ == nullptr) {
    return;
  }
  audio_track_->set_enabled(enabled);
}

bool Sora::GetVideoEnabled() const {
  if (video_track_ == nullptr) {
    return false;
  }
  return video_track_->enabled();
}

void Sora::SetVideoEnabled(bool enabled) {
  if (video_track_ == nullptr) {
    return;
  }
  video_track_->set_enabled(enabled);
}

std::string Sora::GetSelectedSignalingURL() const {
  if (signaling_ == nullptr) {
    return "";
  }
  return signaling_->GetSelectedSignalingURL();
}

std::string Sora::GetConnectedSignalingURL() const {
  if (signaling_ == nullptr) {
    return "";
  }
  return signaling_->GetConnectedSignalingURL();
}

}  // namespace sora_unity_sdk
