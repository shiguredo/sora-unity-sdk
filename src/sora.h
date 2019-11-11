#ifndef SORA_SORA_H_INCLUDED
#define SORA_SORA_H_INCLUDED

#include <memory>
#include <string>
#include <thread>

// boost
#include <boost/asio/io_context.hpp>

// webrtc
#include "rtc_base/log_sinks.h"

#if __APPLE__
#include "mac_helper/mac_capturer.h"
#else
#include "rtc/device_video_capturer.h"
#endif

// sora
#include "id_pointer.h"
#include "rtc/rtc_manager.h"
#include "sora_signaling.h"
#include "unity.h"
#include "unity_camera_capturer.h"
#include "unity_context.h"
#include "unity_renderer.h"

namespace sora {

class Sora {
  boost::asio::io_context ioc_;
  UnityContext* context_;
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

  rtc::scoped_refptr<UnityCameraCapturer> unity_camera_capturer_;
  ptrid_t ptrid_;

 public:
  Sora(UnityContext* context);
  ~Sora();
  void SetOnAddTrack(std::function<void(ptrid_t)> on_add_track);
  void SetOnRemoveTrack(std::function<void(ptrid_t)> on_remove_track);
  void DispatchEvents();

  bool Connect(std::string signaling_url,
               std::string channel_id,
               std::string metadata,
               bool downstream,
               bool multistream,
               int capturer_type,
               void* unity_camera_texture,
               int video_width,
               int video_height);

  static void UNITY_INTERFACE_API RenderCallbackStatic(int event_id);
  int GetRenderCallbackEventID() const;

  void RenderCallback();
};

}  // namespace sora

#endif  // SORA_SORA_H_INCLUDED
