#ifndef SORA_UNITY_SDK_SORA_H_INCLUDED
#define SORA_UNITY_SDK_SORA_H_INCLUDED

#include <memory>
#include <string>
#include <thread>

// Sora
#include <sora/sora_client_context.h>
#include <sora/sora_signaling.h>

// Boost
#include <boost/asio/io_context.hpp>

// WebRTC
#include <api/environment/environment_factory.h>
#include <api/scoped_refptr.h>
#include <api/task_queue/task_queue_factory.h>
#include <media/engine/webrtc_media_engine.h>
#include <modules/audio_device/include/audio_device.h>
#include <pc/connection_context.h>

#include "id_pointer.h"
#include "sora_conf.json.h"
#include "sora_conf_internal.json.h"
#include "unity.h"
#include "unity_audio_device.h"
#include "unity_camera_capturer.h"
#include "unity_context.h"
#include "unity_renderer.h"

#ifdef SORA_UNITY_SDK_ANDROID
#include <sdk/android/native_api/jni/scoped_java_ref.h>
#endif

namespace sora_unity_sdk {

class Sora : public std::enable_shared_from_this<Sora>,
             public sora::SoraSignalingObserver {
 public:
  Sora(UnityContext* context);
  ~Sora();

  void SetOnAddTrack(std::function<void(ptrid_t, std::string)> on_add_track);
  void SetOnRemoveTrack(
      std::function<void(ptrid_t, std::string)> on_remove_track);
  void SetOnMediaStreamTrack(
      std::function<void(webrtc::RtpTransceiverInterface*,
                         webrtc::MediaStreamTrackInterface*,
                         const std::string&)> on_media_stream_track);
  void SetOnRemoveMediaStreamTrack(
      std::function<void(webrtc::RtpReceiverInterface*,
                         webrtc::MediaStreamTrackInterface*,
                         const std::string&)> on_remove_media_stream_track);
  void SetOnSetOffer(std::function<void(std::string)> on_set_offer);
  void SetOnNotify(std::function<void(std::string)> on_notify);
  void SetOnPush(std::function<void(std::string)> on_push);
  void SetOnMessage(std::function<void(std::string, std::string)> on_message);
  void SetOnDisconnect(std::function<void(int, std::string)> on_disconnect);
  void SetOnDataChannel(std::function<void(std::string)> on_data_channel);
  void SetOnCapturerFrame(std::function<void(std::string)> on_capturer_frame);
  void DispatchEvents();

  void Connect(const sora_conf::internal::ConnectConfig& cc);
  void Disconnect();
  void SwitchCamera(const sora_conf::internal::CameraConfig& cc);

  static void UNITY_INTERFACE_API RenderCallbackStatic(int event_id);
  int GetRenderCallbackEventID() const;

  void RenderCallback();

  webrtc::VideoTrackInterface* GetVideoTrackFromVideoSinkId(
      ptrid_t video_sink_id) const;
  ptrid_t GetVideoSinkIdFromVideoTrack(
      webrtc::VideoTrackInterface* video_track) const;

  void ProcessAudio(const void* p, int offset, int samples);
  void SetOnHandleAudio(std::function<void(const int16_t*, int, int)> f);
  void SetSenderAudioSink(webrtc::AudioTrackSinkInterface* sink);

  void GetStats(std::function<void(std::string)> on_get_stats);

  void SendMessage(const std::string& label, const std::string& data);

  bool GetAudioEnabled() const;
  void SetAudioEnabled(bool enabled);
  bool GetVideoEnabled() const;
  void SetVideoEnabled(bool enabled);

  std::string GetSelectedSignalingURL() const;
  std::string GetConnectedSignalingURL() const;

 private:
  void* GetAndroidApplicationContext(void* env);
  static sora_conf::ErrorCode ToErrorCode(sora::SoraSignalingErrorCode ec);

  // SoraSignalingObserver の実装
  void OnSetOffer(std::string offer) override;
  void OnDisconnect(sora::SoraSignalingErrorCode ec,
                    std::string message) override;
  void OnNotify(std::string text) override;
  void OnPush(std::string text) override;
  void OnMessage(std::string label, std::string data) override;
  void OnTrack(webrtc::scoped_refptr<webrtc::RtpTransceiverInterface>
                   transceiver) override;
  void OnRemoveTrack(
      webrtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override;
  void OnDataChannel(std::string label) override;

 private:
  void DoConnect(const sora_conf::internal::ConnectConfig& config,
                 std::function<void(int, std::string)> on_disconnect);
  void DoSwitchCamera(
      const sora_conf::internal::CameraConfig& cc,
      webrtc::scoped_refptr<webrtc::VideoTrackInterface> video_track);

  static webrtc::scoped_refptr<UnityAudioDevice> CreateADM(
      webrtc::Environment env,
      bool dummy_audio,
      bool unity_audio_input,
      bool unity_audio_output,
      std::function<void(const int16_t*, int, int)> on_handle_audio,
      webrtc::AudioTrackSinkInterface* sink,
      std::string audio_recording_device,
      std::string audio_playout_device,
      webrtc::Thread* worker_thread,
      void* jni_env,
      void* android_context);
  static bool InitADM(webrtc::scoped_refptr<webrtc::AudioDeviceModule> adm,
                      std::string audio_recording_device,
                      std::string audio_playout_device);

  static webrtc::scoped_refptr<webrtc::VideoTrackSourceInterface>
  CreateVideoCapturer(
      int capturer_type,
      void* unity_camera_texture,
      bool no_video_device,
      std::string video_capturer_device,
      int video_width,
      int video_height,
      int video_fps,
      std::function<void(const webrtc::VideoFrame& frame)> on_frame,
      webrtc::Thread* signaling_thread,
      void* jni_env,
      void* android_context,
      UnityContext* unity_context);

  void PushEvent(std::function<void()> f);

  struct CapturerSink : webrtc::VideoSinkInterface<webrtc::VideoFrame> {
    CapturerSink(
        webrtc::scoped_refptr<webrtc::VideoTrackSourceInterface> capturer,
        std::function<void(std::string)> on_frame);
    ~CapturerSink() override;
    void OnFrame(const webrtc::VideoFrame& frame) override;

   private:
    webrtc::scoped_refptr<webrtc::VideoTrackSourceInterface> capturer_;
    std::function<void(std::string)> on_frame_;
  };

 private:
  std::unique_ptr<boost::asio::io_context> ioc_;
  std::shared_ptr<sora::SoraSignaling> signaling_;
  UnityContext* unity_context_;
  std::unique_ptr<UnityRenderer> renderer_;
  webrtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track_;
  webrtc::scoped_refptr<webrtc::VideoTrackInterface> video_track_;
  webrtc::scoped_refptr<webrtc::RtpSenderInterface> video_sender_;
  std::function<void(ptrid_t, std::string)> on_add_track_;
  std::function<void(ptrid_t, std::string)> on_remove_track_;
  std::function<void(webrtc::RtpTransceiverInterface*,
                     webrtc::MediaStreamTrackInterface*,
                     const std::string&)>
      on_media_stream_track_;
  std::function<void(webrtc::RtpReceiverInterface*,
                     webrtc::MediaStreamTrackInterface*,
                     const std::string&)>
      on_remove_media_stream_track_;
  std::function<void(std::string)> on_set_offer_;
  std::function<void(std::string)> on_notify_;
  std::function<void(std::string)> on_push_;
  std::function<void(std::string, std::string)> on_message_;
  std::function<void(int, std::string)> on_disconnect_;
  std::function<void(std::string)> on_data_channel_;
  std::function<void(const int16_t*, int, int)> on_handle_audio_;
  std::function<void(std::string)> on_capturer_frame_;

  webrtc::AudioTrackSinkInterface* sender_audio_sink_ = nullptr;

  std::shared_ptr<sora::SoraClientContext> sora_context_;
  std::unique_ptr<webrtc::Thread> io_thread_;

  std::mutex event_mutex_;
  std::deque<std::function<void()>> event_queue_;

  ptrid_t ptrid_;

  std::map<std::string, std::string> connection_ids_;

  std::string stream_id_;

  webrtc::scoped_refptr<webrtc::VideoTrackSourceInterface> capturer_;
  int capturer_type_ = 0;
  std::shared_ptr<CapturerSink> capturer_sink_;

  webrtc::scoped_refptr<UnityAudioDevice> unity_adm_;
  webrtc::TaskQueueFactory* task_queue_factory_;
#if defined(SORA_UNITY_SDK_ANDROID)
  webrtc::ScopedJavaGlobalRef<jobject> android_context_;
#endif

  std::atomic<bool> set_offer_ = false;
};

}  // namespace sora_unity_sdk

#endif
