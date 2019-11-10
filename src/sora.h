#ifndef SORA_SORA_H_INCLUDED
#define SORA_SORA_H_INCLUDED

#include <boost/asio/io_context.hpp>
#include <memory>
#include <string>
#include <thread>

#include "rtc/rtc_manager.h"
#include "rtc_base/log_sinks.h"

#if __APPLE__
#include "mac_helper/mac_capturer.h"
#else
#include "rtc/device_video_capturer.h"
#endif

#include "id_pointer.h"
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
  Sora(UnityContext* context) : ioc_(1), context_(context) {
    ptrid_ = IdPointer::Instance().Register(this);
  }

  ~Sora() {
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
  void SetOnAddTrack(std::function<void(ptrid_t)> on_add_track) {
    on_add_track_ = on_add_track;
  }
  void SetOnRemoveTrack(std::function<void(ptrid_t)> on_remove_track) {
    on_remove_track_ = on_remove_track;
  }
  void DispatchEvents() {
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

  bool Connect(std::string signaling_url,
               std::string channel_id,
               bool downstream,
               bool multistream,
               int capturer_type,
               void* unity_camera_texture,
               int video_width,
               int video_height) {
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

    RTC_LOG(LS_INFO) << "Sora::Connect signaling_url=" << signaling_url
                     << " channel_id=" << channel_id
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
      rtc_manager_.reset(
          new RTCManager(config, std::move(capturer), renderer_.get()));
    } else {
      // 受信側は capturer を作らず、video, recording の設定もしない
      RTCManagerConfig config;
      config.no_recording = true;
      config.no_video = true;
      rtc_manager_.reset(new RTCManager(config, nullptr, renderer_.get()));
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
      signaling_ = SoraSignaling::Create(ioc_, rtc_manager_.get(), config);
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

  static void UNITY_INTERFACE_API RenderCallbackStatic(int event_id) {
    auto sora = (sora::Sora*)IdPointer::Instance().Lookup(event_id);
    if (sora == nullptr) {
      return;
    }

    sora->RenderCallback();
  }
  int GetRenderCallbackEventID() const { return ptrid_; }

  void RenderCallback() {
    if (!unity_camera_capturer_) {
      return;
    }
    unity_camera_capturer_->OnRender();
  }
};

}  // namespace sora

#endif  // SORA_SORA_H_INCLUDED