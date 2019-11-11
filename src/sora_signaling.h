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
#include <nlohmann/json.hpp>

#include "rtc/rtc_manager.h"
#include "rtc/rtc_message_sender.h"
#include "url_parts.h"

namespace sora {

struct SoraSignalingConfig {
  std::string signaling_url;
  std::string channel_id;

  nlohmann::json metadata;
  std::string video_codec = "VP9";
  int video_bitrate = 0;

  std::string audio_codec = "OPUS";
  int audio_bitrate = 0;

  enum class Role { Upstream, Downstream };
  Role role = Role::Upstream;
  bool multistream = false;
};

class SoraSignaling : public std::enable_shared_from_this<SoraSignaling>,
                      public RTCMessageSender {
  boost::asio::io_context& ioc_;

  boost::asio::ip::tcp::resolver resolver_;

  typedef boost::beast::websocket::stream<
      boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>
      ssl_websocket_t;
  std::unique_ptr<ssl_websocket_t> wss_;
  boost::beast::multi_buffer read_buffer_;
  std::vector<boost::beast::flat_buffer> write_buffer_;

  URLParts parts_;

  RTCManager* manager_;
  std::shared_ptr<RTCConnection> connection_;
  SoraSignalingConfig config_;
  std::function<void(std::string)> on_notify_;

  webrtc::PeerConnectionInterface::IceConnectionState rtc_state_;

  bool connected_ = false;
  bool answer_sent_ = false;

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
  bool connect();
  void close();

  // connection_ = nullptr すると直ちに onIceConnectionStateChange コールバックが呼ばれるが、
  // この中で使っている shared_from_this() がデストラクタ内で使えないため、デストラクタで connection_ = nullptr すると実行時エラーになる。
  // なのでこのクラスを解放する前に明示的に release() 関数を呼んでもらうことにする。.
  void release();

 private:
  void onResolve(boost::system::error_code ec,
                 boost::asio::ip::tcp::resolver::results_type results);
  void onSSLConnect(boost::system::error_code ec);
  void onSSLHandshake(boost::system::error_code ec);
  void onHandshake(boost::system::error_code ec);

  void doSendConnect();
  void doSendPong();
  void createPeerFromConfig(nlohmann::json jconfig);

 private:
  void onClose(boost::system::error_code ec);

  void doRead();
  void onRead(boost::system::error_code ec, std::size_t bytes_transferred);

  void sendText(std::string text);
  void doSendText(std::string text);

  void doWrite();
  void onWrite(boost::system::error_code ec, std::size_t bytes_transferred);

 private:
  // WebRTC からのコールバック
  // これらは別スレッドからやってくるので取り扱い注意.
  void onIceConnectionStateChange(
      webrtc::PeerConnectionInterface::IceConnectionState new_state) override;
  void onIceCandidate(const std::string sdp_mid,
                      const int sdp_mlineindex,
                      const std::string sdp) override;
  void onCreateDescription(webrtc::SdpType type,
                           const std::string sdp) override;
  void onSetDescription(webrtc::SdpType type) override;

 private:
  void doIceConnectionStateChange(
      webrtc::PeerConnectionInterface::IceConnectionState new_state);
};
}  // namespace sora

#endif  // SORA_SORA_SIGNALING_H_
