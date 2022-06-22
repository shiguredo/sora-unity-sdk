#include "sora.h"

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
#include "mac_helper/ios_audio_init.h"
#endif

namespace sora_unity_sdk {

Sora::Sora(UnityContext* context) : context_(context) {
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
  capturer_ = nullptr;
  unity_adm_ = nullptr;

  audio_track_ = nullptr;
  video_track_ = nullptr;
  connection_context_ = nullptr;
  factory_ = nullptr;

  if (ioc_ != nullptr) {
    ioc_->stop();
  }
  signaling_.reset();
  ioc_.reset();
  if (io_thread_) {
    io_thread_->Stop();
    io_thread_.reset();
  }
  if (network_thread_) {
    network_thread_->Stop();
    network_thread_.reset();
  }
  if (worker_thread_) {
    worker_thread_->Stop();
    worker_thread_.reset();
  }
  if (signaling_thread_) {
    signaling_thread_->Stop();
    signaling_thread_.reset();
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

  IosAudioInit(
      [this, on_disconnect = std::move(on_disconnect)](std::string error) {
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

  rtc::InitializeSSL();

  network_thread_ = rtc::Thread::CreateWithSocketServer();
  network_thread_->Start();
  worker_thread_ = rtc::Thread::Create();
  worker_thread_->Start();
  signaling_thread_ = rtc::Thread::Create();
  signaling_thread_->Start();

  webrtc::PeerConnectionFactoryDependencies dependencies;
  dependencies.network_thread = network_thread_.get();
  dependencies.worker_thread = worker_thread_.get();
  dependencies.signaling_thread = signaling_thread_.get();
  dependencies.task_queue_factory = webrtc::CreateDefaultTaskQueueFactory();
  dependencies.call_factory = webrtc::CreateCallFactory();
  dependencies.event_log_factory =
      absl::make_unique<webrtc::RtcEventLogFactory>(
          dependencies.task_queue_factory.get());

  // worker 上の env
  auto worker_env = worker_thread_->Invoke<void*>(
      RTC_FROM_HERE, [] { return sora::GetJNIEnv(); });
  // worker 上の context
#if defined(SORA_UNITY_SDK_ANDROID)
  void* worker_context =
      worker_thread_->Invoke<jobject>(RTC_FROM_HERE, [worker_env] {
        return ::sora_unity_sdk::GetAndroidApplicationContext(
                   (JNIEnv*)worker_env)
            .Release();
      });
#else
  void* worker_context = nullptr;
#endif

  // media_dependencies
  cricket::MediaEngineDependencies media_dependencies;
  media_dependencies.task_queue_factory = dependencies.task_queue_factory.get();
  auto adm =
      CreateADM(media_dependencies.task_queue_factory, false,
                cc.unity_audio_input, cc.unity_audio_output, on_handle_audio_,
                cc.audio_recording_device, cc.audio_playout_device,
                worker_thread_.get(), worker_env, worker_context);
  media_dependencies.adm = adm;

#if defined(SORA_UNITY_SDK_ANDROID)
  worker_thread_->Invoke<void>(RTC_FROM_HERE, [worker_env, worker_context] {
    ((JNIEnv*)worker_env)->DeleteLocalRef((jobject)worker_context);
  });
#endif

  media_dependencies.audio_encoder_factory =
      webrtc::CreateBuiltinAudioEncoderFactory();
  media_dependencies.audio_decoder_factory =
      webrtc::CreateBuiltinAudioDecoderFactory();

  void* env = sora::GetJNIEnv();
  void* android_context = GetAndroidApplicationContext(env);

  auto cuda_context = sora::CudaContext::Create();
  {
    auto config = sora::GetDefaultVideoEncoderFactoryConfig(cuda_context, env);
    config.use_simulcast_adapter = true;
    media_dependencies.video_encoder_factory =
        absl::make_unique<sora::SoraVideoEncoderFactory>(std::move(config));
  }
  {
    auto config = sora::GetDefaultVideoDecoderFactoryConfig(cuda_context, env);
    media_dependencies.video_decoder_factory =
        absl::make_unique<sora::SoraVideoDecoderFactory>(std::move(config));
  }

  media_dependencies.audio_mixer = nullptr;
  media_dependencies.audio_processing =
      webrtc::AudioProcessingBuilder().Create();

  dependencies.media_engine =
      cricket::CreateMediaEngine(std::move(media_dependencies));

  factory_ = sora::CreateModularPeerConnectionFactoryWithContext(
      std::move(dependencies), connection_context_);

  if (factory_ == nullptr) {
    RTC_LOG(LS_ERROR) << "Failed to create PeerConnectionFactory";
    on_disconnect((int)sora_conf::ErrorCode::INTERNAL_ERROR,
                  "Failed to create PeerConnectionFactory");
    return;
  }

  webrtc::PeerConnectionFactoryInterface::Options factory_options;
  factory_options.disable_encryption = false;
  factory_options.ssl_max_version = rtc::SSL_PROTOCOL_DTLS_12;
  factory_options.crypto_options.srtp.enable_gcm_crypto_suites = true;
  factory_->SetOptions(factory_options);

  if (!InitADM(adm, cc.audio_recording_device, cc.audio_playout_device)) {
    on_disconnect((int)sora_conf::ErrorCode::INTERNAL_ERROR,
                  "Failed to InitADM");
    return;
  }

  renderer_.reset(new UnityRenderer());

  if (cc.role == "sendonly" || cc.role == "sendrecv") {
    auto capturer = CreateVideoCapturer(
        cc.capturer_type, (void*)cc.unity_camera_texture,
        cc.video_capturer_device, cc.video_width, cc.video_height,
        signaling_thread_.get(), env, android_context);
    if (!capturer) {
      on_disconnect((int)sora_conf::ErrorCode::INTERNAL_ERROR,
                    "Capturer Init Failed");
      return;
    }

    capturer_ = capturer;
    capturer_type_ = cc.capturer_type;

    std::string audio_track_id = rtc::CreateRandomString(16);
    audio_track_ = factory_->CreateAudioTrack(
        audio_track_id, factory_->CreateAudioSource(cricket::AudioOptions()));
    std::string video_track_id = rtc::CreateRandomString(16);
    video_track_ = factory_->CreateVideoTrack(video_track_id, capturer.get());
    auto track_id = renderer_->AddTrack(video_track_.get());
    PushEvent([this, track_id]() {
      if (on_add_track_) {
        on_add_track_(track_id);
      }
    });
  }

  {
    RTC_LOG(LS_INFO) << "Start Signaling: cc=" << jsonif::to_json(cc);
    ioc_.reset(new boost::asio::io_context(1));
    sora::SoraSignalingConfig config;
    config.observer = shared_from_this();
    config.pc_factory = factory_;
    config.io_context = ioc_.get();
    config.role = cc.role;
    if (cc.enable_multistream) {
      config.multistream = cc.multistream;
    }
    if (cc.enable_spotlight) {
      config.spotlight = cc.spotlight;
    }
    config.spotlight_number = cc.spotlight_number;
    config.spotlight_focus_rid = cc.spotlight_focus_rid;
    config.spotlight_unfocus_rid = cc.spotlight_unfocus_rid;
    if (cc.enable_simulcast) {
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
    for (const auto& dc : cc.data_channels) {
      sora::SoraSignalingConfig::DataChannel d;
      d.label = dc.label;
      d.direction = dc.direction;
      if (dc.enable_ordered) {
        d.ordered = dc.ordered;
      }
      if (dc.enable_max_packet_life_time) {
        d.max_packet_life_time = dc.max_packet_life_time;
      }
      if (dc.enable_max_retransmits) {
        d.max_retransmits = dc.max_retransmits;
      }
      if (dc.enable_protocol) {
        d.protocol = dc.protocol;
      }
      if (dc.enable_compress) {
        d.compress = dc.compress;
      }
      config.data_channels.push_back(std::move(d));
    }
    if (!cc.metadata.empty()) {
      boost::json::error_code ec;
      auto md = boost::json::parse(cc.metadata, ec);
      if (ec) {
        RTC_LOG(LS_WARNING) << "Invalid JSON: metadata=" << cc.metadata;
      } else {
        config.metadata = md;
      }
    }
    config.insecure = cc.insecure;
    config.proxy_url = cc.proxy_url;
    config.proxy_username = cc.proxy_username;
    config.proxy_password = cc.proxy_password;
    config.proxy_agent = cc.proxy_agent;
    config.network_manager = connection_context_->default_network_manager();
    config.socket_factory = connection_context_->default_socket_factory();

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
    adm = worker_thread->Invoke<rtc::scoped_refptr<webrtc::AudioDeviceModule>>(
        RTC_FROM_HERE, [&] {
          return webrtc::AudioDeviceModule::Create(
              webrtc::AudioDeviceModule::kDummyAudio, task_queue_factory);
        });
  } else {
    sora::AudioDeviceModuleConfig config;
    config.audio_layer = webrtc::AudioDeviceModule::kPlatformDefaultAudio;
    config.task_queue_factory = task_queue_factory;
    config.jni_env = jni_env;
    config.application_context = android_context;
    adm = worker_thread->Invoke<rtc::scoped_refptr<webrtc::AudioDeviceModule>>(
        RTC_FROM_HERE, [&] { return sora::CreateAudioDeviceModule(config); });
  }

  return worker_thread->Invoke<rtc::scoped_refptr<UnityAudioDevice>>(
      RTC_FROM_HERE, [&] {
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
    std::string video_capturer_device,
    int video_width,
    int video_height,
    rtc::Thread* signaling_thread,
    void* jni_env,
    void* android_context) {
  if (capturer_type == 0) {
    // 実カメラ（デバイス）を使う
    sora::CameraDeviceCapturerConfig config;
    config.width = video_width;
    config.height = video_height;
    // TODO(melpon): framerate をちゃんと設定する
    config.fps = 30;
    config.device_name = video_capturer_device;
    config.jni_env = jni_env;
    config.application_context = android_context;
    config.signaling_thread = signaling_thread;

    return sora::CreateCameraDeviceCapturer(config);
  } else {
    // Unity のカメラからの映像を使う
    return UnityCameraCapturer::Create(&UnityContext::Instance(),
                                       unity_camera_texture, video_width,
                                       video_height);
  }
}

void Sora::GetStats(std::function<void(std::string)> on_get_stats) {
  auto pc = signaling_ == nullptr ? nullptr : signaling_->GetPeerConnection();
  if (signaling_ == nullptr || pc == nullptr) {
    PushEvent(
        [on_get_stats = std::move(on_get_stats)]() { on_get_stats("[]"); });
    return;
  }

  pc->GetStats(sora::RTCStatsCallback::Create(
      [self = shared_from_this(), on_get_stats = std::move(on_get_stats)](
          const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report) {
        std::string json = report->ToJson();
        self->PushEvent(
            [on_get_stats = std::move(on_get_stats), json = std::move(json)]() {
              on_get_stats(std::move(json));
            });
      }));
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

void Sora::OnSetOffer() {
  std::string stream_id = rtc::CreateRandomString(16);
  if (audio_track_ != nullptr) {
    webrtc::RTCErrorOr<rtc::scoped_refptr<webrtc::RtpSenderInterface>>
        audio_result = signaling_->GetPeerConnection()->AddTrack(audio_track_,
                                                                 {stream_id});
  }
  if (video_track_ != nullptr) {
    webrtc::RTCErrorOr<rtc::scoped_refptr<webrtc::RtpSenderInterface>>
        video_result = signaling_->GetPeerConnection()->AddTrack(video_track_,
                                                                 {stream_id});
  }
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
    auto track_id = renderer_->AddTrack(
        static_cast<webrtc::VideoTrackInterface*>(track.get()));
    PushEvent([this, track_id]() {
      if (on_add_track_) {
        on_add_track_(track_id);
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

    if (track_id != 0) {
      PushEvent([this, track_id]() {
        if (on_remove_track_) {
          on_remove_track_(track_id);
        }
      });
    }
  }
}

void Sora::PushEvent(std::function<void()> f) {
  std::lock_guard<std::mutex> guard(event_mutex_);
  event_queue_.push_back(std::move(f));
}
}  // namespace sora_unity_sdk
