#ifndef SORA_SORA_SIGNALING_H_
#define SORA_SORA_SIGNALING_H_

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <memory>
#include <string>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <boost/json.hpp>

#include "rtc/rtc_manager.h"
#include "rtc/rtc_message_sender.h"
#include "sora_data_channel_on_asio.h"
#include "url_parts.h"
#include "watchdog.h"
#include "websocket.h"

namespace sora {

struct SoraSignalingConfig {
  std::string unity_version;
  std::string signaling_url;
  std::string channel_id;

  boost::json::value metadata;

  bool video = true;
  std::string video_codec_type = "";
  int video_bit_rate = 0;

  bool audio = true;
  std::string audio_codec_type = "";
  int audio_bit_rate = 0;

  enum class Role { Sendonly, Recvonly, Sendrecv };
  Role role = Role::Sendonly;
  bool multistream = false;
  bool spotlight = false;
  int spotlight_number = 0;
  // デフォルトは rid を設定しないため unspecified (未指定)
  std::string spotlight_focus_rid = "unspecified";
  // デフォルトは rid を設定しないため unspecified (未指定)
  std::string spotlight_unfocus_rid = "unspecified";
  bool simulcast = false;
  bool data_channel_signaling = false;
  int data_channel_signaling_timeout = 180;
  bool ignore_disconnect_websocket = false;
  bool close_websocket = true;

  bool insecure = false;
};

class SoraSignaling : public std::enable_shared_from_this<SoraSignaling>,
                      public RTCMessageSender,
                      public SoraDataChannelObserver {
  boost::asio::io_context& ioc_;
  std::shared_ptr<Websocket> ws_;
  std::shared_ptr<SoraDataChannelOnAsio> dc_;
  bool ignore_disconnect_websocket_;

  URLParts parts_;

  RTCManager* manager_;
  std::shared_ptr<RTCConnection> connection_;
  SoraSignalingConfig config_;
  std::function<void(std::string)> on_notify_;
  std::function<void(std::string)> on_push_;

  webrtc::PeerConnectionInterface::IceConnectionState rtc_state_;

  bool connected_ = false;
  bool destructed_ = false;

  WatchDog watchdog_;

 public:
  webrtc::PeerConnectionInterface::IceConnectionState getRTCConnectionState()
      const;
  std::shared_ptr<RTCConnection> getRTCConnection() const;

  static std::shared_ptr<SoraSignaling> Create(
      boost::asio::io_context& ioc,
      RTCManager* manager,
      SoraSignalingConfig config,
      std::function<void(std::string)> on_notify,
      std::function<void(std::string)> on_push);

 private:
  SoraSignaling(boost::asio::io_context& ioc,
                RTCManager* manager,
                SoraSignalingConfig config,
                std::function<void(std::string)> on_notify,
                std::function<void(std::string)> on_push);
  bool Init();

 public:
  ~SoraSignaling();

 public:
  bool Connect();
  void Close(std::function<void()> on_close);

 private:
  void OnWatchdogExpired();

  void OnConnect(boost::system::error_code ec);

  void DoSendConnect();
  void DoSendPong();
  void DoSendPong(
      const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report);
  void DoSendUpdate(const std::string& sdp, std::string type);
  std::shared_ptr<sora::RTCConnection> CreateRTCConnection(
      boost::json::value jconfig);

 private:
  void DoRead();
  void OnRead(boost::system::error_code ec,
              std::size_t bytes_transferred,
              std::string text);

 private:
  // DataChannel 周りのコールバック
  void OnStateChange(
      rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) override;
  void OnMessage(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel,
                 const webrtc::DataBuffer& buffer) override;

 private:
  // WebRTC からのコールバック
  // これらは別スレッドからやってくるので取り扱い注意.
  void OnIceConnectionStateChange(
      webrtc::PeerConnectionInterface::IceConnectionState new_state) override;
  void OnIceCandidate(const std::string sdp_mid,
                      const int sdp_mlineindex,
                      const std::string sdp) override;

 private:
  void DoIceConnectionStateChange(
      webrtc::PeerConnectionInterface::IceConnectionState new_state);
};
}  // namespace sora

#endif  // SORA_SORA_SIGNALING_H_
