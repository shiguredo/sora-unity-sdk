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
#include "url_parts.h"
#include "websocket.h"

namespace sora {

struct SoraSignalingConfig {
  std::string unity_version;
  std::string signaling_url;
  std::string channel_id;

  boost::json::value metadata;
  std::string video_codec = "VP9";
  int video_bitrate = 0;

  std::string audio_codec = "OPUS";
  int audio_bitrate = 0;

  enum class Role { Sendonly, Recvonly, Sendrecv };
  Role role = Role::Sendonly;
  bool multistream = false;

  bool insecure = false;
};

class SoraSignaling : public std::enable_shared_from_this<SoraSignaling>,
                      public RTCMessageSender {
  boost::asio::io_context& ioc_;
  std::unique_ptr<Websocket> ws_;

  URLParts parts_;

  RTCManager* manager_;
  std::shared_ptr<RTCConnection> connection_;
  SoraSignalingConfig config_;
  std::function<void(std::string)> on_notify_;

  webrtc::PeerConnectionInterface::IceConnectionState rtc_state_;

  bool connected_ = false;

 public:
  webrtc::PeerConnectionInterface::IceConnectionState getRTCConnectionState()
      const;
  std::shared_ptr<RTCConnection> getRTCConnection() const;

  static std::shared_ptr<SoraSignaling> Create(
      boost::asio::io_context& ioc,
      RTCManager* manager,
      SoraSignalingConfig config,
      std::function<void(std::string)> on_notify);

 private:
  SoraSignaling(boost::asio::io_context& ioc,
                RTCManager* manager,
                SoraSignalingConfig config,
                std::function<void(std::string)> on_notify);
  bool Init();

 public:
  bool Connect();
  void Close();

  // connection_ = nullptr すると直ちに onIceConnectionStateChange コールバックが呼ばれるが、
  // この中で使っている shared_from_this() がデストラクタ内で使えないため、デストラクタで connection_ = nullptr すると実行時エラーになる。
  // なのでこのクラスを解放する前に明示的に Release() 関数を呼んでもらうことにする。.
  void Release();

 private:
  void OnConnect(boost::system::error_code ec);

  void DoSendConnect();
  void DoSendPong();
  void DoSendPong(
      const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report);
  void CreatePeerFromConfig(boost::json::value jconfig);

 private:
  void OnClose(boost::system::error_code ec);

  void DoRead();
  void OnRead(boost::system::error_code ec,
              std::size_t bytes_transferred,
              std::string text);

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
