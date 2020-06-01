#include "sora_signaling.h"
#include "sora_version.h"

#include <boost/asio/connect.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <nlohmann/json.hpp>

namespace {

std::string iceConnectionStateToString(
    webrtc::PeerConnectionInterface::IceConnectionState state) {
  switch (state) {
    case webrtc::PeerConnectionInterface::kIceConnectionNew:
      return "new";
    case webrtc::PeerConnectionInterface::kIceConnectionChecking:
      return "checking";
    case webrtc::PeerConnectionInterface::kIceConnectionConnected:
      return "connected";
    case webrtc::PeerConnectionInterface::kIceConnectionCompleted:
      return "completed";
    case webrtc::PeerConnectionInterface::kIceConnectionFailed:
      return "failed";
    case webrtc::PeerConnectionInterface::kIceConnectionDisconnected:
      return "disconnected";
    case webrtc::PeerConnectionInterface::kIceConnectionClosed:
      return "closed";
    case webrtc::PeerConnectionInterface::kIceConnectionMax:
      return "max";
  }
  return "unknown";
}

}  // namespace

using json = nlohmann::json;

namespace sora {

webrtc::PeerConnectionInterface::IceConnectionState
SoraSignaling::getRTCConnectionState() const {
  return rtc_state_;
}

std::shared_ptr<RTCConnection> SoraSignaling::getRTCConnection() const {
  if (rtc_state_ == webrtc::PeerConnectionInterface::IceConnectionState::
                        kIceConnectionConnected) {
    return connection_;
  } else {
    return nullptr;
  }
}

std::shared_ptr<SoraSignaling> SoraSignaling::Create(
    boost::asio::io_context& ioc,
    RTCManager* manager,
    SoraSignalingConfig config,
    std::function<void(std::string)> on_notify) {
  auto p = std::shared_ptr<SoraSignaling>(
      new SoraSignaling(ioc, manager, config, std::move(on_notify)));
  if (!p->Init()) {
    return nullptr;
  }
  return p;
}

SoraSignaling::SoraSignaling(boost::asio::io_context& ioc,
                             RTCManager* manager,
                             SoraSignalingConfig config,
                             std::function<void(std::string)> on_notify)
    : ioc_(ioc),
      resolver_(ioc),
      manager_(manager),
      config_(config),
      on_notify_(std::move(on_notify)) {}

bool SoraSignaling::Init() {
  if (!URLParts::parse(config_.signaling_url, parts_)) {
    RTC_LOG(LS_ERROR) << "Invalid Signaling URL: " << config_.signaling_url;
    return false;
  }

  if (parts_.scheme != "wss") {
    RTC_LOG(LS_ERROR) << "Signaling URL Scheme is not secure web socket (wss): "
                      << config_.signaling_url;
    return false;
  }

  boost::asio::ssl::context ssl_ctx(boost::asio::ssl::context::tlsv12);
  ssl_ctx.set_default_verify_paths();
  ssl_ctx.set_options(boost::asio::ssl::context::default_workarounds |
                      boost::asio::ssl::context::no_sslv2 |
                      boost::asio::ssl::context::no_sslv3 |
                      boost::asio::ssl::context::single_dh_use);

  wss_.reset(new ssl_websocket_t(ioc_, ssl_ctx));
  wss_->write_buffer_bytes(8192);

  // SNI の設定を行う
  if (!SSL_set_tlsext_host_name(wss_->next_layer().native_handle(),
                                parts_.host.c_str())) {
    boost::system::error_code ec{static_cast<int>(::ERR_get_error()),
                                 boost::asio::error::get_ssl_category()};
    RTC_LOG(LS_ERROR) << "Failed SSL_set_tlsext_host_name: ec=" << ec;
    return false;
  }

  return true;
}

void SoraSignaling::release() {
  // connection_ を nullptr にした上で解放する
  // デストラクタ中にコールバックが呼ばれて解放中の connection_ にアクセスしてしまうことがあるため
  auto connection = std::move(connection_);
  connection = nullptr;
}

bool SoraSignaling::connect() {
  RTC_LOG(LS_INFO) << __FUNCTION__;

  if (connected_) {
    return false;
  }

  std::string port = "443";
  if (!parts_.port.empty()) {
    port = parts_.port;
  }

  RTC_LOG(LS_INFO) << "Start to resolve DNS: host=" << parts_.host
                   << " port=" << port;

  // DNS ルックアップ
  resolver_.async_resolve(parts_.host, port,
                          boost::beast::bind_front_handler(
                              &SoraSignaling::onResolve, shared_from_this()));

  return true;
}

void SoraSignaling::onResolve(
    boost::system::error_code ec,
    boost::asio::ip::tcp::resolver::results_type results) {
  RTC_LOG(LS_INFO) << __FUNCTION__;

  if (ec) {
    RTC_LOG(LS_ERROR) << "Failed to resolve DNS: " << ec;
    return;
  }

  // DNS ルックアップで得られたエンドポイントに対して接続する
  boost::asio::async_connect(
      wss_->next_layer().next_layer(), results.begin(), results.end(),
      std::bind(&SoraSignaling::onSSLConnect, shared_from_this(),
                std::placeholders::_1));
}

void SoraSignaling::onSSLConnect(boost::system::error_code ec) {
  RTC_LOG(LS_INFO) << __FUNCTION__;

  if (ec) {
    RTC_LOG(LS_ERROR) << "Failed to connect: " << ec;
    return;
  }

  // SSL のハンドシェイク
  wss_->next_layer().async_handshake(
      boost::asio::ssl::stream_base::client,
      boost::beast::bind_front_handler(&SoraSignaling::onSSLHandshake,
                                       shared_from_this()));
}

void SoraSignaling::onSSLHandshake(boost::system::error_code ec) {
  RTC_LOG(LS_INFO) << __FUNCTION__;

  if (ec) {
    RTC_LOG(LS_ERROR) << "Failed SSL handshake: " << ec;
    return;
  }

  // Websocket のハンドシェイク
  wss_->async_handshake(parts_.host, parts_.path_query_fragment,
                        boost::beast::bind_front_handler(
                            &SoraSignaling::onHandshake, shared_from_this()));
}

void SoraSignaling::onHandshake(boost::system::error_code ec) {
  RTC_LOG(LS_INFO) << __FUNCTION__;

  if (ec) {
    RTC_LOG(LS_ERROR) << "Failed Websocket handshake: " << ec;
    return;
  }

  connected_ = true;
  RTC_LOG(LS_INFO) << "Signaling Websocket is connected";

  doRead();
  doSendConnect();
}

#define SORA_CLIENT                                                        \
  "Sora Unity SDK for " SORA_UNITY_SDK_PLATFORM " " SORA_UNITY_SDK_VERSION \
  " (" SORA_UNITY_SDK_COMMIT_SHORT ")"
#define LIBWEBRTC                                                      \
  "Shiguredo-build " WEBRTC_READABLE_VERSION " (" WEBRTC_BUILD_VERSION \
  " " WEBRTC_SRC_COMMIT_SHORT ")"

void SoraSignaling::doSendConnect() {
  std::string role =
    config_.role == SoraSignalingConfig::Role::Sendonly ? "sendonly" :
    config_.role == SoraSignalingConfig::Role::Recvonly ? "recvonly" : "sendrecv";

  json json_message = {
      {"type", "connect"},
      {"role", role},
      {"multistream", config_.multistream},
      {"channel_id", config_.channel_id},
      {"sora_client", SORA_CLIENT},
      {"libwebrtc", LIBWEBRTC},
      {"environment", "Unity " + config_.unity_version},
  };

  if (!config_.metadata.is_null()) {
    json_message["metadata"] = config_.metadata;
  }

  json_message["video"]["codec_type"] = config_.video_codec;
  if (config_.video_bitrate != 0) {
    json_message["video"]["bit_rate"] = config_.video_bitrate;
  }

  json_message["audio"]["codec_type"] = config_.audio_codec;
  if (config_.audio_bitrate != 0) {
    json_message["audio"]["bit_rate"] = config_.audio_bitrate;
  }

  sendText(json_message.dump());
}
void SoraSignaling::doSendPong() {
  json json_message = {{"type", "pong"}};
  sendText(json_message.dump());
}
void SoraSignaling::doSendPong(
    const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report) {
  std::string stats = report->ToJson();
  json json_message = {{"type", "pong"}, {"stats", stats}};
  std::string str = R"({"type":"pong","stats":)" + stats + "}";
  sendText(std::move(str));
}

void SoraSignaling::createPeerFromConfig(json jconfig) {
  webrtc::PeerConnectionInterface::RTCConfiguration rtc_config;
  webrtc::PeerConnectionInterface::IceServers ice_servers;

  auto jservers = jconfig["iceServers"];
  for (auto jserver : jservers) {
    const std::string username = jserver["username"];
    const std::string credential = jserver["credential"];
    auto jurls = jserver["urls"];
    for (const std::string url : jurls) {
      webrtc::PeerConnectionInterface::IceServer ice_server;
      ice_server.uri = url;
      ice_server.username = username;
      ice_server.password = credential;
      ice_servers.push_back(ice_server);
    }
  }

  rtc_config.servers = ice_servers;

  connection_ = manager_->createConnection(rtc_config, this);
}

void SoraSignaling::close() {
  wss_->async_close(boost::beast::websocket::close_code::normal,
                    boost::beast::bind_front_handler(&SoraSignaling::onClose,
                                                     shared_from_this()));
}

void SoraSignaling::onClose(boost::system::error_code ec) {
  if (ec) {
    RTC_LOG(LS_ERROR) << "Failed to close: ec=" << ec;
    return;
  }
}

void SoraSignaling::doRead() {
  wss_->async_read(read_buffer_,
                   boost::beast::bind_front_handler(&SoraSignaling::onRead,
                                                    shared_from_this()));
}

void SoraSignaling::onRead(boost::system::error_code ec,
                           std::size_t bytes_transferred) {
  boost::ignore_unused(bytes_transferred);

  if (ec == boost::asio::error::operation_aborted) {
    return;
  }

  if (ec) {
    RTC_LOG(LS_ERROR) << "Failed to read: ec=" << ec;
    return;
  }

  const auto text = boost::beast::buffers_to_string(read_buffer_.data());
  read_buffer_.consume(read_buffer_.size());

  RTC_LOG(LS_INFO) << __FUNCTION__ << ": text=" << text;

  auto json_message = json::parse(text);
  const std::string type = json_message["type"];
  if (type == "offer") {
    answer_sent_ = false;
    createPeerFromConfig(json_message["config"]);
    const std::string sdp = json_message["sdp"];
    connection_->setOffer(sdp);
  } else if (type == "update") {
    const std::string sdp = json_message["sdp"];
    connection_->setOffer(sdp);
  } else if (type == "notify") {
    if (on_notify_) {
      on_notify_(text);
    }
  } else if (type == "ping") {
    if (rtc_state_ != webrtc::PeerConnectionInterface::IceConnectionState::
                          kIceConnectionConnected) {
      doRead();
      return;
    }
    bool stats = json_message.value("stats", false);
    if (stats) {
      connection_->getStats(
          [this](
              const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report) {
            doSendPong(report);
          });
    } else {
      doSendPong();
    }
  }

  doRead();
}

void SoraSignaling::sendText(std::string text) {
  RTC_LOG(LS_INFO) << __FUNCTION__;

  boost::asio::post(boost::beast::bind_front_handler(
      &SoraSignaling::doSendText, shared_from_this(), std::move(text)));
}

void SoraSignaling::doSendText(std::string text) {
  RTC_LOG(LS_INFO) << __FUNCTION__ << ": " << text;

  bool empty = write_buffer_.empty();
  boost::beast::flat_buffer buffer;

  const auto n = boost::asio::buffer_copy(buffer.prepare(text.size()),
                                          boost::asio::buffer(text));
  buffer.commit(n);

  write_buffer_.push_back(std::move(buffer));

  if (empty) {
    doWrite();
  }
}

void SoraSignaling::doWrite() {
  RTC_LOG(LS_INFO) << __FUNCTION__;

  auto& buffer = write_buffer_.front();

  wss_->text(true);
  wss_->async_write(buffer.data(),
                    boost::beast::bind_front_handler(&SoraSignaling::onWrite,
                                                     shared_from_this()));
}

void SoraSignaling::onWrite(boost::system::error_code ec,
                            std::size_t bytes_transferred) {
  RTC_LOG(LS_INFO) << __FUNCTION__;

  if (ec == boost::asio::error::operation_aborted) {
    return;
  }

  if (ec) {
    RTC_LOG(LS_ERROR) << "Failed to write: ec=" << ec;
    return;
  }

  write_buffer_.erase(write_buffer_.begin());

  if (!write_buffer_.empty()) {
    doWrite();
  }
}

// WebRTC からのコールバック
// これらは別スレッドからやってくるので取り扱い注意
void SoraSignaling::onIceConnectionStateChange(
    webrtc::PeerConnectionInterface::IceConnectionState new_state) {
  RTC_LOG(LS_INFO) << __FUNCTION__ << " state:" << new_state;
  boost::asio::post(boost::beast::bind_front_handler(
      &SoraSignaling::doIceConnectionStateChange, shared_from_this(),
      new_state));
}
void SoraSignaling::onIceCandidate(const std::string sdp_mid,
                                   const int sdp_mlineindex,
                                   const std::string sdp) {
  json json_message = {{"type", "candidate"}, {"candidate", sdp}};
  sendText(json_message.dump());
}
void SoraSignaling::onCreateDescription(webrtc::SdpType type,
                                        const std::string sdp) {
  // 最初の1回目は answer を送り、以降は update を送る
  if (!answer_sent_) {
    answer_sent_ = true;
    json json_message = {{"type", "answer"}, {"sdp", sdp}};
    sendText(json_message.dump());
  } else {
    json json_message = {{"type", "update"}, {"sdp", sdp}};
    sendText(json_message.dump());
  }
}
void SoraSignaling::onSetDescription(webrtc::SdpType type) {
  RTC_LOG(LS_INFO) << __FUNCTION__
                   << " SdpType: " << webrtc::SdpTypeToString(type);
  if (connection_ && type == webrtc::SdpType::kOffer) {
    connection_->createAnswer();
  }
}

void SoraSignaling::doIceConnectionStateChange(
    webrtc::PeerConnectionInterface::IceConnectionState new_state) {
  RTC_LOG(LS_INFO) << __FUNCTION__
                   << ": oldState=" << iceConnectionStateToString(rtc_state_)
                   << ", newState=" << iceConnectionStateToString(new_state);

  switch (new_state) {
    case webrtc::PeerConnectionInterface::IceConnectionState::
        kIceConnectionConnected:
      break;
    case webrtc::PeerConnectionInterface::IceConnectionState::
        kIceConnectionFailed:
      break;
    default:
      break;
  }
  rtc_state_ = new_state;
}

}  // namespace sora
