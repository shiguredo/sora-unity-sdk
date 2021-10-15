#include <iostream>

// WebRTC
#include <absl/memory/memory.h>
#include <api/audio_codecs/builtin_audio_decoder_factory.h>
#include <api/audio_codecs/builtin_audio_encoder_factory.h>
#include <api/create_peerconnection_factory.h>
#include <api/rtc_event_log/rtc_event_log_factory.h>
#include <api/task_queue/default_task_queue_factory.h>
#include <api/video_codecs/builtin_video_decoder_factory.h>
#include <api/video_codecs/builtin_video_encoder_factory.h>
#include <pc/video_track_source_proxy.h>
#include <media/engine/webrtc_media_engine.h>
#include <modules/audio_device/include/audio_device.h>
#include <modules/audio_device/include/audio_device_factory.h>
#include <modules/audio_processing/include/audio_processing.h>
#include <modules/video_capture/video_capture.h>
#include <modules/video_capture/video_capture_factory.h>
#include <rtc_base/logging.h>
#include <rtc_base/ssl_adapter.h>

#include "peer_connection_observer.h"
#include "rtc_manager.h"
#include "rtc_ssl_verifier.h"
#include "scalable_track_source.h"

#if defined(SORA_UNITY_SDK_MACOS) || defined(SORA_UNITY_SDK_IOS)
#include "mac_helper/objc_codec_factory_helper.h"
#elif defined(SORA_UNITY_SDK_ANDROID)
#include "android_helper/android_codec_factory_helper.h"
#else
#include "hw_video_decoder_factory.h"
#include "hw_video_encoder_factory.h"
#endif
#include "simulcast_video_encoder_factory.h"

namespace {

std::string GenerateRandomChars(size_t length) {
  std::string result;
  rtc::CreateRandomString(length, &result);
  return result;
}

std::string GenerateRandomChars() {
  return GenerateRandomChars(32);
}

}  // namespace

namespace sora {

std::unique_ptr<RTCManager> RTCManager::Create(
    RTCManagerConfig config,
    rtc::scoped_refptr<rtc::AdaptedVideoTrackSource> video_track_source,
    VideoTrackReceiver* receiver,
    rtc::scoped_refptr<webrtc::AudioDeviceModule> adm,
    std::unique_ptr<webrtc::TaskQueueFactory> task_queue_factory,
    std::unique_ptr<rtc::Thread> signaling_thread,
    std::unique_ptr<rtc::Thread> worker_thread) {
  std::unique_ptr<RTCManager> p(new RTCManager());
  if (!p->Init(config, video_track_source, receiver, adm,
               std::move(task_queue_factory), std::move(signaling_thread),
               std::move(worker_thread))) {
    return nullptr;
  }
  return p;
}

RTCManager::RTCManager() {}

bool RTCManager::Init(
    RTCManagerConfig config,
    rtc::scoped_refptr<rtc::AdaptedVideoTrackSource> video_track_source,
    VideoTrackReceiver* receiver,
    rtc::scoped_refptr<webrtc::AudioDeviceModule> adm,
    std::unique_ptr<webrtc::TaskQueueFactory> task_queue_factory,
    std::unique_ptr<rtc::Thread> signaling_thread,
    std::unique_ptr<rtc::Thread> worker_thread) {
  config_ = config;
  receiver_ = receiver;

  rtc::InitializeSSL();

  network_thread_ = rtc::Thread::CreateWithSocketServer();
  network_thread_->Start();
  worker_thread_ = std::move(worker_thread);
  signaling_thread_ = std::move(signaling_thread);
  signaling_thread_->Start();

  webrtc::PeerConnectionFactoryDependencies dependencies;
  dependencies.network_thread = network_thread_.get();
  dependencies.worker_thread = worker_thread_.get();
  dependencies.signaling_thread = signaling_thread_.get();
  dependencies.task_queue_factory = std::move(task_queue_factory);
  dependencies.call_factory = webrtc::CreateCallFactory();
  dependencies.event_log_factory =
      absl::make_unique<webrtc::RtcEventLogFactory>(
          dependencies.task_queue_factory.get());

  // media_dependencies
  cricket::MediaEngineDependencies media_dependencies;
  media_dependencies.task_queue_factory = dependencies.task_queue_factory.get();

  media_dependencies.adm = adm;

  media_dependencies.audio_encoder_factory =
      webrtc::CreateBuiltinAudioEncoderFactory();
  media_dependencies.audio_decoder_factory =
      webrtc::CreateBuiltinAudioDecoderFactory();
#if defined(SORA_UNITY_SDK_MACOS) || defined(SORA_UNITY_SDK_IOS)
  media_dependencies.video_encoder_factory =
      absl::make_unique<SimulcastVideoEncoderFactory>(
          CreateObjCEncoderFactory());
  media_dependencies.video_decoder_factory = CreateObjCDecoderFactory();
#elif defined(SORA_UNITY_SDK_ANDROID)
  JNIEnv* jni = webrtc::AttachCurrentThreadIfNeeded();
  media_dependencies.video_encoder_factory =
      absl::make_unique<SimulcastVideoEncoderFactory>(
          CreateAndroidEncoderFactory(jni));
  media_dependencies.video_decoder_factory = CreateAndroidDecoderFactory(jni);
#else
  media_dependencies.video_encoder_factory =
      absl::make_unique<SimulcastVideoEncoderFactory>(
          absl::make_unique<HWVideoEncoderFactory>());
  media_dependencies.video_decoder_factory =
      absl::make_unique<HWVideoDecoderFactory>();
#endif
  media_dependencies.audio_mixer = nullptr;
  media_dependencies.audio_processing =
      webrtc::AudioProcessingBuilder().Create();

  dependencies.media_engine =
      cricket::CreateMediaEngine(std::move(media_dependencies));

  factory_ =
      webrtc::CreateModularPeerConnectionFactory(std::move(dependencies));
  if (!factory_.get()) {
    RTC_LOG(LS_ERROR) << __FUNCTION__
                      << ": Failed to initialize PeerConnectionFactory";
    return false;
  }

  if (!InitADM(adm, config.audio_recording_device,
               config.audio_playout_device)) {
    return false;
  }

  webrtc::PeerConnectionFactoryInterface::Options factory_options;
  factory_options.disable_encryption = false;
  factory_options.ssl_max_version = rtc::SSL_PROTOCOL_DTLS_12;
  factory_options.crypto_options.srtp.enable_gcm_crypto_suites = true;
  factory_->SetOptions(factory_options);

  if (!config_.no_recording) {
    cricket::AudioOptions ao;
    if (config_.disable_echo_cancellation)
      ao.echo_cancellation = false;
    if (config_.disable_auto_gain_control)
      ao.auto_gain_control = false;
    if (config_.disable_noise_suppression)
      ao.noise_suppression = false;
    if (config_.disable_highpass_filter)
      ao.highpass_filter = false;
    if (config_.disable_typing_detection)
      ao.typing_detection = false;
    RTC_LOG(LS_INFO) << __FUNCTION__ << ": " << ao.ToString();
    audio_track_ = factory_->CreateAudioTrack(GenerateRandomChars(),
                                              factory_->CreateAudioSource(ao));
    if (!audio_track_) {
      RTC_LOG(LS_ERROR) << __FUNCTION__ << ": Cannot create audio_track";
      return false;
    }
  }

  if (video_track_source && !config_.no_video) {
    rtc::scoped_refptr<webrtc::VideoTrackSourceInterface> video_source =
        webrtc::VideoTrackSourceProxy::Create(
            signaling_thread_.get(), worker_thread_.get(), video_track_source);
    video_track_ =
        factory_->CreateVideoTrack(GenerateRandomChars(), video_source);
    if (video_track_) {
      if (config_.fixed_resolution) {
        video_track_->set_content_hint(
            webrtc::VideoTrackInterface::ContentHint::kText);
      }
      if (receiver != nullptr) {
        receiver->AddTrack(video_track_.get());
      }
    } else {
      RTC_LOG(LS_ERROR) << __FUNCTION__ << ": Cannot create video_track";
      return false;
    }
  }
  return true;
}

bool RTCManager::InitADM(rtc::scoped_refptr<webrtc::AudioDeviceModule> adm,
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

RTCManager::~RTCManager() {
  RTC_LOG(LS_INFO) << "RTCManager::~RTCManager started";
  audio_track_ = nullptr;
  video_track_ = nullptr;
  factory_ = nullptr;
  network_thread_->Stop();
  worker_thread_->Stop();
  signaling_thread_->Stop();

  rtc::CleanupSSL();
  RTC_LOG(LS_INFO) << "RTCManager::~RTCManager completed";
}

void RTCManager::SetDataManager(std::shared_ptr<RTCDataManager> data_manager) {
  data_manager_proxy_.SetDataManager(data_manager);
}

std::shared_ptr<RTCConnection> RTCManager::CreateConnection(
    webrtc::PeerConnectionInterface::RTCConfiguration rtc_config,
    RTCMessageSender* sender) {
  rtc_config.enable_dtls_srtp = true;
  rtc_config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
  std::unique_ptr<PeerConnectionObserver> observer(
      new PeerConnectionObserver(sender, receiver_, &data_manager_proxy_));
  webrtc::PeerConnectionDependencies dependencies(observer.get());

  // WebRTC の SSL 接続の検証は自前のルート証明書(rtc_base/ssl_roots.h)でやっていて、
  // その中に Let's Encrypt の証明書が無いため、接続先によっては接続できないことがある。
  //
  // それを解消するために tls_cert_verifier を設定して自前で検証を行う。
  dependencies.tls_cert_verifier = std::unique_ptr<rtc::SSLCertificateVerifier>(
      new RTCSSLVerifier(config_.insecure));

  webrtc::RTCErrorOr<rtc::scoped_refptr<webrtc::PeerConnectionInterface>>
      connection = factory_->CreatePeerConnectionOrError(
          rtc_config, std::move(dependencies));
  if (!connection.ok()) {
    RTC_LOG(LS_ERROR) << __FUNCTION__ << ": CreatePeerConnection failed";
    return nullptr;
  }

  return std::make_shared<RTCConnection>(sender, std::move(observer),
                                         connection.value());
}

void RTCManager::InitTracks(RTCConnection* conn) {
  auto connection = conn->GetConnection();

  std::string stream_id = GenerateRandomChars();

  if (audio_track_) {
    webrtc::RTCErrorOr<rtc::scoped_refptr<webrtc::RtpSenderInterface>>
        audio_sender = connection->AddTrack(audio_track_, {stream_id});
    if (!audio_sender.ok()) {
      RTC_LOG(LS_WARNING) << __FUNCTION__ << ": Cannot add audio_track_";
    }
  }

  if (video_track_) {
    webrtc::RTCErrorOr<rtc::scoped_refptr<webrtc::RtpSenderInterface>>
        video_add_result = connection->AddTrack(video_track_, {stream_id});
    if (video_add_result.ok()) {
      rtc::scoped_refptr<webrtc::RtpSenderInterface> video_sender =
          video_add_result.value();
      webrtc::RtpParameters parameters = video_sender->GetParameters();
      parameters.degradation_preference = config_.priority;
      video_sender->SetParameters(parameters);
    } else {
      RTC_LOG(LS_WARNING) << __FUNCTION__ << ": Cannot add video_track_";
    }
  }
}

}  // namespace sora
