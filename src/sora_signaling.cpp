#include "sora_signaling.h"
#include "sora_version.h"

#include <boost/asio/connect.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <boost/json.hpp>
#include <boost/preprocessor/stringize.hpp>

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
    std::function<void(std::string)> on_notify,
    std::function<void(std::string)> on_push) {
  auto p = std::shared_ptr<SoraSignaling>(new SoraSignaling(
      ioc, manager, config, std::move(on_notify), std::move(on_push)));
  if (!p->Init()) {
    return nullptr;
  }
  return p;
}

SoraSignaling::SoraSignaling(boost::asio::io_context& ioc,
                             RTCManager* manager,
                             SoraSignalingConfig config,
                             std::function<void(std::string)> on_notify,
                             std::function<void(std::string)> on_push)
    : ioc_(ioc),
      manager_(manager),
      config_(config),
      on_notify_(std::move(on_notify)),
      on_push_(std::move(on_push)) {}

bool SoraSignaling::Init() {
  return true;
}

void SoraSignaling::Release() {
  // connection_ を nullptr にした上で解放する
  // デストラクタ中にコールバックが呼ばれて解放中の connection_ にアクセスしてしまうことがあるため
  auto connection = std::move(connection_);
  connection = nullptr;
}

bool SoraSignaling::Connect() {
  RTC_LOG(LS_INFO) << __FUNCTION__;

  if (connected_) {
    return false;
  }

  std::string port = "443";
  if (!parts_.port.empty()) {
    port = parts_.port;
  }

  RTC_LOG(LS_INFO) << "Connect to " << parts_.host;

  URLParts parts;
  if (!URLParts::Parse(config_.signaling_url, parts)) {
    RTC_LOG(LS_ERROR) << "Invalid Signaling URL: " << config_.signaling_url;
    return false;
  }
  if (parts.scheme == "ws") {
    ws_.reset(new Websocket(ioc_));
  } else if (parts.scheme == "wss") {
    ws_.reset(new Websocket(Websocket::ssl_tag(), ioc_, config_.insecure));
  } else {
    return false;
  }

  ws_->Connect(config_.signaling_url,
               std::bind(&SoraSignaling::OnConnect, shared_from_this(),
                         std::placeholders::_1));

  return true;
}

void SoraSignaling::OnConnect(boost::system::error_code ec) {
  RTC_LOG(LS_INFO) << __FUNCTION__;

  if (ec) {
    RTC_LOG(LS_ERROR) << "Failed Websocket handshake: " << ec;
    return;
  }

  connected_ = true;
  RTC_LOG(LS_INFO) << "Signaling Websocket is connected";

  DoRead();
  DoSendConnect();
}

#define SORA_CLIENT \
  "Sora Unity SDK " SORA_UNITY_SDK_VERSION " (" SORA_UNITY_SDK_COMMIT_SHORT ")"
#define LIBWEBRTC                                                      \
  "Shiguredo-build " WEBRTC_READABLE_VERSION " (" WEBRTC_BUILD_VERSION \
  " " WEBRTC_SRC_COMMIT_SHORT ")"

void SoraSignaling::DoSendConnect() {
  std::string role =
      config_.role == SoraSignalingConfig::Role::Sendonly   ? "sendonly"
      : config_.role == SoraSignalingConfig::Role::Recvonly ? "recvonly"
                                                            : "sendrecv";

  boost::json::object json_message = {
      {"type", "connect"},
      {"role", role},
      {"multistream", config_.multistream},
      {"spotlight", config_.spotlight},
      {"simulcast", config_.simulcast},
      {"channel_id", config_.channel_id},
      {"sora_client", SORA_CLIENT},
      {"libwebrtc", LIBWEBRTC},
      {"environment",
       "Unity " + config_.unity_version + " for " SORA_UNITY_SDK_PLATFORM},
  };

  if (!config_.metadata.is_null()) {
    json_message["metadata"] = config_.metadata;
  }

  json_message["video"] = boost::json::object();
  json_message["video"].as_object()["codec_type"] = config_.video_codec;
  if (config_.video_bitrate != 0) {
    json_message["video"].as_object()["bit_rate"] = config_.video_bitrate;
  }

  json_message["audio"] = boost::json::object();
  json_message["audio"].as_object()["codec_type"] = config_.audio_codec;
  if (config_.audio_bitrate != 0) {
    json_message["audio"].as_object()["bit_rate"] = config_.audio_bitrate;
  }

  if (config_.spotlight && config_.spotlight_number > 0) {
    json_message["spotlight_number"] = config_.spotlight_number;
  }

  ws_->WriteText(boost::json::serialize(json_message));
}
void SoraSignaling::DoSendPong() {
  boost::json::value json_message = {{"type", "pong"}};
  ws_->WriteText(boost::json::serialize(json_message));
}
void SoraSignaling::DoSendPong(
    const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report) {
  std::string stats = report->ToJson();
  std::string str = R"({"type":"pong","stats":)" + stats + "}";
  ws_->WriteText(std::move(str));
}

std::shared_ptr<sora::RTCConnection> SoraSignaling::CreateRTCConnection(
    boost::json::value jconfig) {
  webrtc::PeerConnectionInterface::RTCConfiguration rtc_config;
  webrtc::PeerConnectionInterface::IceServers ice_servers;

  auto jservers = jconfig.at("iceServers");
  for (auto jserver : jservers.as_array()) {
    const std::string username = jserver.at("username").as_string().c_str();
    const std::string credential = jserver.at("credential").as_string().c_str();
    auto jurls = jserver.at("urls");
    for (const auto url : jurls.as_array()) {
      webrtc::PeerConnectionInterface::IceServer ice_server;
      ice_server.uri = url.as_string().c_str();
      ice_server.username = username;
      ice_server.password = credential;
      ice_servers.push_back(ice_server);
    }
  }

  rtc_config.servers = ice_servers;

  return manager_->CreateConnection(rtc_config, this);
}

void SoraSignaling::Close() {
  ws_->Close(std::bind(&SoraSignaling::OnClose, shared_from_this(),
                       std::placeholders::_1));
}

void SoraSignaling::OnClose(boost::system::error_code ec) {
  if (ec) {
    RTC_LOG(LS_ERROR) << "Failed to close: ec=" << ec;
    return;
  }
}

void SoraSignaling::DoRead() {
  ws_->Read(std::bind(&SoraSignaling::OnRead, shared_from_this(),
                      std::placeholders::_1, std::placeholders::_2,
                      std::placeholders::_3));
}

void SoraSignaling::OnRead(boost::system::error_code ec,
                           std::size_t bytes_transferred,
                           std::string text) {
  boost::ignore_unused(bytes_transferred);

  if (ec == boost::asio::error::operation_aborted) {
    return;
  }

  if (ec) {
    RTC_LOG(LS_ERROR) << "Failed to read: ec=" << ec;
    return;
  }

  RTC_LOG(LS_INFO) << __FUNCTION__ << ": text=" << text;

  auto json_message = boost::json::parse(text);
  const std::string type = json_message.at("type").as_string().c_str();
  if (type == "offer") {
    connection_ = CreateRTCConnection(json_message.at("config"));
    const std::string sdp = json_message.at("sdp").as_string().c_str();
    connection_->SetOffer(sdp, [self = shared_from_this(), json_message]() {
      boost::asio::post([self, json_message]() {
        if (!self->connection_) {
          return;
        }

        // simulcast では offer の setRemoteDescription が終わった後に
        // トラックを追加する必要があるため、ここで初期化する
        self->manager_->InitTracks(self->connection_.get());

        if (self->config_.simulcast &&
            json_message.as_object().count("encodings") != 0) {
          std::vector<webrtc::RtpEncodingParameters> encoding_parameters;

          // "encodings" キーの各内容を webrtc::RtpEncodingParameters に変換する
          auto encodings_json = json_message.at("encodings");
          for (auto v : encodings_json.as_array()) {
            auto p = v.as_object();
            webrtc::RtpEncodingParameters params;
            // absl::optional<uint32_t> ssrc;
            // double bitrate_priority = kDefaultBitratePriority;
            // enum class Priority { kVeryLow, kLow, kMedium, kHigh };
            // Priority network_priority = Priority::kLow;
            // absl::optional<int> max_bitrate_bps;
            // absl::optional<int> min_bitrate_bps;
            // absl::optional<double> max_framerate;
            // absl::optional<int> num_temporal_layers;
            // absl::optional<double> scale_resolution_down_by;
            // bool active = true;
            // std::string rid;
            // bool adaptive_ptime = false;
            params.rid = p.at("rid").as_string().c_str();
            if (p.count("maxBitrate") != 0) {
              params.max_bitrate_bps = p["maxBitrate"].to_number<int>();
            }
            if (p.count("minBitrate") != 0) {
              params.min_bitrate_bps = p["minBitrate"].to_number<int>();
            }
            if (p.count("scaleResolutionDownBy") != 0) {
              params.scale_resolution_down_by =
                  p["scaleResolutionDownBy"].to_number<double>();
            }
            if (p.count("maxFramerate") != 0) {
              params.max_framerate = p["maxFramerate"].to_number<double>();
            }
            if (p.count("active") != 0) {
              params.active = p["active"].as_bool();
            }
            if (p.count("adaptivePtime") != 0) {
              params.adaptive_ptime = p["adaptivePtime"].as_bool();
            }
            encoding_parameters.push_back(params);
          }
          self->connection_->SetEncodingParameters(
              std::move(encoding_parameters));
        }

        self->connection_->CreateAnswer(
            [self](webrtc::SessionDescriptionInterface* desc) {
              std::string sdp;
              desc->ToString(&sdp);
              boost::asio::post(self->ioc_, [self, sdp]() {
                if (!self->connection_) {
                  return;
                }
                boost::json::value json_message = {{"type", "answer"},
                                                   {"sdp", sdp}};
                self->ws_->WriteText(boost::json::serialize(json_message));
              });
            });
      });
    });
  } else if (type == "update") {
    const std::string sdp = json_message.at("sdp").as_string().c_str();
    connection_->SetOffer(sdp, [self = shared_from_this()]() {
      boost::asio::post([self]() {
        if (!self->connection_) {
          return;
        }

        // エンコーディングパラメータの情報がクリアされるので設定し直す
        if (self->config_.simulcast) {
          self->connection_->ResetEncodingParameters();
        }

        self->connection_->CreateAnswer(
            [self](webrtc::SessionDescriptionInterface* desc) {
              std::string sdp;
              desc->ToString(&sdp);
              boost::asio::post([self, sdp]() {
                boost::json::value json_message = {{"type", "update"},
                                                   {"sdp", sdp}};
                self->ws_->WriteText(boost::json::serialize(json_message));
              });
            });
      });
    });
  } else if (type == "notify") {
    if (on_notify_) {
      on_notify_(text);
    }
  } else if (type == "push") {
    if (on_push_) {
      on_push_(text);
    }
  } else if (type == "ping") {
    if (rtc_state_ != webrtc::PeerConnectionInterface::IceConnectionState::
                          kIceConnectionConnected) {
      DoRead();
      return;
    }
    auto it = json_message.as_object().find("stats");
    if (it != json_message.as_object().end() && it->value().as_bool()) {
      connection_->GetStats(
          [this](
              const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report) {
            DoSendPong(report);
          });
    } else {
      DoSendPong();
    }
  }

  DoRead();
}

// WebRTC からのコールバック
// これらは別スレッドからやってくるので取り扱い注意
void SoraSignaling::OnIceConnectionStateChange(
    webrtc::PeerConnectionInterface::IceConnectionState new_state) {
  RTC_LOG(LS_INFO) << __FUNCTION__ << " state:" << new_state;
  boost::asio::post(boost::beast::bind_front_handler(
      &SoraSignaling::DoIceConnectionStateChange, shared_from_this(),
      new_state));
}
void SoraSignaling::OnIceCandidate(const std::string sdp_mid,
                                   const int sdp_mlineindex,
                                   const std::string sdp) {
  boost::json::value json_message = {{"type", "candidate"}, {"candidate", sdp}};
  ws_->WriteText(boost::json::serialize(json_message));
}
void SoraSignaling::DoIceConnectionStateChange(
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
