#ifndef SORA_SORA_H_INCLUDED
#define SORA_SORA_H_INCLUDED

#include <memory>
#include <string>
#include <thread>

// boost
#include <boost/asio/io_context.hpp>

#if defined(SORA_UNITY_SDK_MACOS) || defined(SORA_UNITY_SDK_IOS)
#include "mac_helper/mac_capturer.h"
#else
#include "rtc/device_video_capturer.h"
#endif

// webrtc
#include "api/scoped_refptr.h"
#include "api/task_queue/task_queue_factory.h"
#include "modules/audio_device/include/audio_device.h"

// sora
#include "id_pointer.h"
#include "rtc/rtc_manager.h"
#include "sora_signaling.h"
#include "unity.h"
#include "unity_audio_device.h"
#include "unity_camera_capturer.h"
#include "unity_context.h"
#include "unity_renderer.h"

namespace sora {

class Sora {
  std::unique_ptr<boost::asio::io_context> ioc_;
  UnityContext* context_;
  std::string signaling_url_;
  std::string channel_id_;
  std::unique_ptr<RTCManager> rtc_manager_;
  std::shared_ptr<SoraSignaling> signaling_;
  std::unique_ptr<rtc::Thread> thread_;
  std::unique_ptr<UnityRenderer> renderer_;
  std::function<void(ptrid_t)> on_add_track_;
  std::function<void(ptrid_t)> on_remove_track_;
  std::function<void(std::string)> on_notify_;
  std::function<void(const int16_t*, int, int)> on_handle_audio_;

  std::mutex event_mutex_;
  std::deque<std::function<void()>> event_queue_;

  ptrid_t ptrid_;

  rtc::scoped_refptr<rtc::AdaptedVideoTrackSource> capturer_;
  int capturer_type_ = 0;

  rtc::scoped_refptr<UnityAudioDevice> unity_adm_;

 public:
  Sora(UnityContext* context);
  ~Sora();
  void SetOnAddTrack(std::function<void(ptrid_t)> on_add_track);
  void SetOnRemoveTrack(std::function<void(ptrid_t)> on_remove_track);
  void SetOnNotify(std::function<void(std::string)> on_notify);
  void DispatchEvents();

  struct ConnectConfig {
    std::string unity_version;
    std::string signaling_url;
    std::string channel_id;
    std::string metadata;
    std::string role;
    bool multistream;
    bool spotlight;
    int spotlight_number;
    bool simulcast;
    int capturer_type;
    void* unity_camera_texture;
    std::string video_capturer_device;
    int video_width;
    int video_height;
    std::string video_codec;
    int video_bitrate;
    bool unity_audio_input;
    bool unity_audio_output;
    std::string audio_recording_device;
    std::string audio_playout_device;
    std::string audio_codec;
    int audio_bitrate;
  };

  bool Connect(const ConnectConfig& config);

  static void UNITY_INTERFACE_API RenderCallbackStatic(int event_id);
  int GetRenderCallbackEventID() const;

  void RenderCallback();

  void ProcessAudio(const void* p, int offset, int samples);
  void SetOnHandleAudio(std::function<void(const int16_t*, int, int)> f);

  void GetStats(std::function<void(std::string)> on_get_stats);

 private:
  bool DoConnect(const ConnectConfig& config);

  static rtc::scoped_refptr<UnityAudioDevice> CreateADM(
      webrtc::TaskQueueFactory* task_queue_factory,
      bool dummy_audio,
      bool unity_audio_input,
      bool unity_audio_output,
      std::function<void(const int16_t*, int, int)> on_handle_audio,
      std::string audio_recording_device,
      std::string audio_playout_device,
      rtc::Thread* worker_thread);

  static rtc::scoped_refptr<rtc::AdaptedVideoTrackSource> CreateVideoCapturer(
      int capturer_type,
      void* unity_camera_texture,
      std::string video_capturer_device,
      int video_width,
      int video_height,
      rtc::Thread* signaling_thread);
};

}  // namespace sora

#endif  // SORA_SORA_H_INCLUDED
