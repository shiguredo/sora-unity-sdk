#include "sora_signaling.h"
#include "sora_version.h"
#include "zlib_helper.h"

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
    std::function<void(std::string)> on_push,
    std::function<void(std::string, std::string)> on_message) {
  auto p = std::shared_ptr<SoraSignaling>(
      new SoraSignaling(ioc, manager, config, std::move(on_notify),
                        std::move(on_push), std::move(on_message)));
  if (!p->Init()) {
    return nullptr;
  }
  return p;
}

SoraSignaling::SoraSignaling(
    boost::asio::io_context& ioc,
    RTCManager* manager,
    SoraSignalingConfig config,
    std::function<void(std::string)> on_notify,
    std::function<void(std::string)> on_push,
    std::function<void(std::string, std::string)> on_message)
    : ioc_(ioc),
      manager_(manager),
      config_(config),
      on_notify_(std::move(on_notify)),
      on_push_(std::move(on_push)),
      on_message_(std::move(on_message)) {}

SoraSignaling::~SoraSignaling() {
  RTC_LOG(LS_INFO) << "SoraSignaling::~SoraSignaling started";

  destructed_ = true;
  // 一応閉じる努力はする
  if (using_datachannel_ && dc_) {
    webrtc::DataBuffer disconnect =
        ConvertToDataBuffer("signaling", R"({"type":"disconnect"})");
    dc_->Close(
        disconnect, [dc = dc_]() {}, config_.disconnect_wait_timeout);
    dc_ = nullptr;
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
  }
  // ここで OnIceConnectionStateChange が呼ばれる
  connection_ = nullptr;

  RTC_LOG(LS_INFO) << "SoraSignaling::~SoraSignaling completed";
}
bool SoraSignaling::Init() {
  return true;
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

  dc_.reset(new SoraDataChannelOnAsio(ioc_, this));
  manager_->SetDataManager(dc_);

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

  if (config_.client_id != "") {
    json_message["client_id"] = config_.client_id;
  }

  if (config_.spotlight && config_.spotlight_number > 0) {
    json_message["spotlight_number"] = config_.spotlight_number;
  }

  // 空文字(未指定)の場合は rid を設定しない
  if (config_.spotlight_focus_rid != "") {
    json_message["spotlight_focus_rid"] = config_.spotlight_focus_rid;
  }

  // 空文字(未指定)の場合は rid を設定しない
  if (config_.spotlight_unfocus_rid != "") {
    json_message["spotlight_unfocus_rid"] = config_.spotlight_unfocus_rid;
  }

  // 空文字(未指定)の場合は rid を設定しない
  if (config_.simulcast_rid != "") {
    json_message["simulcast_rid"] = config_.simulcast_rid;
  }

  if (!config_.metadata.is_null()) {
    json_message["metadata"] = config_.metadata;
  }

  if (!config_.video) {
    // video: false の場合はそのまま設定
    json_message["video"] = false;
  } else if (config_.video && config_.video_codec_type.empty() &&
             config_.video_bit_rate == 0) {
    // video: true の場合、その他のオプションの設定が行われてなければ true を設定
    json_message["video"] = true;
  } else {
    // それ以外はちゃんとオプションを設定する
    json_message["video"] = boost::json::object();
    if (!config_.video_codec_type.empty()) {
      json_message["video"].as_object()["codec_type"] =
          config_.video_codec_type;
    }
    if (config_.video_bit_rate != 0) {
      json_message["video"].as_object()["bit_rate"] = config_.video_bit_rate;
    }
  }

  if (!config_.audio) {
    json_message["audio"] = false;
  } else if (config_.audio && config_.audio_codec_type.empty() &&
             config_.audio_bit_rate == 0) {
    json_message["audio"] = true;
  } else {
    json_message["audio"] = boost::json::object();
    if (!config_.audio_codec_type.empty()) {
      json_message["audio"].as_object()["codec_type"] =
          config_.audio_codec_type;
    }
    if (config_.audio_bit_rate != 0) {
      json_message["audio"].as_object()["bit_rate"] = config_.audio_bit_rate;
    }
  }

  if (config_.data_channel_signaling) {
    json_message["data_channel_signaling"] = *config_.data_channel_signaling;
  }
  if (config_.ignore_disconnect_websocket) {
    json_message["ignore_disconnect_websocket"] =
        *config_.ignore_disconnect_websocket;
  }

  if (!config_.data_channel_messaging.empty()) {
    boost::json::array ar;
    for (const auto& m : config_.data_channel_messaging) {
      boost::json::object obj;
      obj["label"] = m.label;
      obj["direction"] = m.direction;
      if (m.enable_ordered) {
        obj["ordered"] = m.ordered;
      }
      if (m.enable_max_packet_life_time) {
        obj["max_packet_life_time"] = m.max_packet_life_time;
      }
      if (m.enable_max_retransmits) {
        obj["max_retransmits"] = m.max_retransmits;
      }
      if (m.enable_protocol) {
        obj["protocol"] = m.protocol;
      }
      if (m.enable_compress) {
        obj["compress"] = m.compress;
      }
      ar.push_back(obj);
    }
    json_message["data_channel_messaging"] = ar;
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
  if (dc_ && using_datachannel_ && dc_->IsOpen("stats")) {
    // DataChannel が使える場合は type: stats で DataChannel に送る
    std::string str = R"({"type":"stats","reports":)" + stats + "}";
    SendDataChannel("stats", str);
  } else if (ws_) {
    std::string str = R"({"type":"pong","stats":)" + stats + "}";
    ws_->WriteText(std::move(str));
  }
}
void SoraSignaling::DoSendUpdate(const std::string& sdp, std::string type) {
  boost::json::value json_message = {{"type", type}, {"sdp", sdp}};
  if (dc_ && using_datachannel_ && dc_->IsOpen("signaling")) {
    // DataChannel が使える場合は DataChannel に送る
    SendDataChannel("signaling", boost::json::serialize(json_message));
  } else if (ws_) {
    ws_->WriteText(boost::json::serialize(json_message));
  }
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

void SoraSignaling::Close(std::function<void()> on_close) {
  auto dc = std::move(dc_);
  dc_ = nullptr;
  auto ws = std::move(ws_);
  ws_ = nullptr;
  auto connection = std::move(connection_);
  connection_ = nullptr;

  auto on_ws_close = [self = shared_from_this(), ws,
                      on_close](boost::system::error_code ec) {
    if (ec) {
      RTC_LOG(LS_ERROR) << "Failed to close WebSocket: ec=" << ec;
    }
    self->connected_ = false;
    self->using_datachannel_ = false;
    on_close();
  };

  if (using_datachannel_ && ws) {
    webrtc::DataBuffer disconnect =
        ConvertToDataBuffer("signaling", R"({"type":"disconnect"})");
    dc->Close(
        disconnect,
        [self = shared_from_this(), dc, connection, ws = std::move(ws),
         on_ws_close]() { ws->Close(on_ws_close); },
        config_.disconnect_wait_timeout);
  } else if (using_datachannel_ && !ws) {
    webrtc::DataBuffer disconnect =
        ConvertToDataBuffer("signaling", R"({"type":"disconnect"})");
    dc->Close(
        disconnect, [dc, connection, on_close]() { on_close(); },
        config_.disconnect_wait_timeout);
  } else if (!using_datachannel_ && ws) {
    boost::json::value disconnect = {{"type", "disconnect"}};
    ws->WriteText(boost::json::serialize(disconnect),
                  [self = shared_from_this(), ws](boost::system::error_code,
                                                  std::size_t) {});
    ws->Close(on_ws_close);
  } else {
    connected_ = false;
    on_close();
  }
}
void SoraSignaling::SendMessage(const std::string& label,
                                const std::string& data) {
  SendDataChannel(label, data);
}

void SoraSignaling::OnWatchdogExpired() {
  Close([]() {});
}

void SoraSignaling::DoRead() {
  ws_->Read([self = shared_from_this(), ws = ws_](boost::system::error_code ec,
                                                  std::size_t bytes_transferred,
                                                  std::string text) {
    self->OnRead(ec, bytes_transferred, std::move(text));
  });
}

void SoraSignaling::OnRead(boost::system::error_code ec,
                           std::size_t bytes_transferred,
                           std::string text) {
  boost::ignore_unused(bytes_transferred);

  if (ec == boost::asio::error::operation_aborted) {
    return;
  }

  if (ec) {
    Close([]() {});
    RTC_LOG(LS_ERROR) << "Failed to read: ec=" << ec;
    return;
  }

  RTC_LOG(LS_INFO) << __FUNCTION__ << ": text=" << text;

  auto json_message = boost::json::parse(text);
  const std::string type = json_message.at("type").as_string().c_str();
  if (type == "offer") {
    // Data Channel の圧縮されたデータが送られてくるラベルを覚えておく
    {
      auto it = json_message.as_object().find("data_channels");
      if (it != json_message.as_object().end()) {
        const auto& ar = it->value().as_array();
        for (const auto& v : ar) {
          if (v.at("compress").as_bool()) {
            compressed_labels_.insert(v.at("label").as_string().c_str());
          }
        }
      }
    }

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
  } else if (type == "update" || type == "re-offer") {
    std::string answer_type = type == "update" ? "update" : "re-answer";
    const std::string sdp = json_message.at("sdp").as_string().c_str();
    connection_->SetOffer(sdp, [self = shared_from_this(), answer_type]() {
      boost::asio::post([self, answer_type]() {
        if (!self->connection_) {
          return;
        }

        // エンコーディングパラメータの情報がクリアされるので設定し直す
        if (self->config_.simulcast) {
          self->connection_->ResetEncodingParameters();
        }

        self->connection_->CreateAnswer(
            [self, answer_type](webrtc::SessionDescriptionInterface* desc) {
              std::string sdp;
              desc->ToString(&sdp);
              self->DoSendUpdate(sdp, answer_type);
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
  } else if (type == "switched") {
    // Data Channel による通信の開始
    using_datachannel_ = true;

    // ignore_disconnect_websocket == true の場合は WS を切断する
    auto it = json_message.as_object().find("ignore_disconnect_websocket");
    if (it != json_message.as_object().end() && it->value().as_bool() && ws_) {
      RTC_LOG(LS_INFO) << "Close WebSocket for DataChannel";
      auto ws = ws_;
      ws_ = nullptr;
      ws->Close([ws](boost::system::error_code) {});
      return;
    }
  }

  DoRead();
}

webrtc::DataBuffer SoraSignaling::ConvertToDataBuffer(
    const std::string& label,
    const std::string& input) {
  bool compressed = compressed_labels_.find(label) != compressed_labels_.end();
  RTC_LOG(LS_INFO) << "Convert to DataChannel label=" << label
                   << " compressed=" << compressed << " input=" << input;
  const std::string& str = compressed ? ZlibHelper::Compress(input) : input;
  return webrtc::DataBuffer(rtc::CopyOnWriteBuffer(str), true);
}

void SoraSignaling::SendDataChannel(const std::string& label,
                                    const std::string& input) {
  webrtc::DataBuffer data = ConvertToDataBuffer(label, input);
  dc_->Send(label, data);
}

void SoraSignaling::OnStateChange(
    rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) {}

void SoraSignaling::OnMessage(
    rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel,
    const webrtc::DataBuffer& buffer) {
  if (!using_datachannel_ || !dc_) {
    return;
  }

  std::string label = data_channel->label();
  bool compressed = compressed_labels_.find(label) != compressed_labels_.end();
  std::string data;
  if (compressed) {
    data = ZlibHelper::Uncompress(buffer.data.cdata(), buffer.size());
  } else {
    data.assign((const char*)buffer.data.cdata(),
                (const char*)buffer.data.cdata() + buffer.size());
  }
  RTC_LOG(LS_INFO) << "label=" << label << " data=" << data;

  // ハンドリングする必要のあるラベル以外は何もしない
  if (label != "signaling" && label != "stats" && label != "push" &&
      label != "notify" && (label.empty() || label[0] != '#')) {
    return;
  }

  // ユーザ定義のラベルは JSON ではないので JSON パース前に処理して終わる
  if (!label.empty() && label[0] == '#') {
    if (on_message_) {
      on_message_(label, data);
    }
    return;
  }

  boost::json::error_code ec;
  auto json = boost::json::parse(data, ec);
  if (ec) {
    RTC_LOG(LS_ERROR) << "JSON Parse Error ec=" << ec.message();
    return;
  }

  if (label == "signaling") {
    const std::string type = json.at("type").as_string().c_str();
    if (type == "re-offer") {
      const std::string sdp = json.at("sdp").as_string().c_str();
      connection_->SetOffer(sdp, [self = shared_from_this()]() {
        boost::asio::post(self->ioc_, [self]() {
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
                boost::asio::post(self->ioc_, [self, sdp]() {
                  if (!self->connection_) {
                    return;
                  }
                  self->DoSendUpdate(sdp, "re-answer");
                });
              });
        });
      });
    }
  }

  if (label == "stats") {
    connection_->GetStats(
        [self = shared_from_this()](
            const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report) {
          self->DoSendPong(report);
        });
  }

  if (label == "notify") {
    if (on_notify_) {
      on_notify_(data);
    }
  }

  if (label == "push") {
    if (on_push_) {
      on_push_(data);
    }
  }
}

// WebRTC からのコールバック
// これらは別スレッドからやってくるので取り扱い注意
void SoraSignaling::OnIceConnectionStateChange(
    webrtc::PeerConnectionInterface::IceConnectionState new_state) {
  RTC_LOG(LS_INFO) << __FUNCTION__ << " state:" << new_state;
  // デストラクタだと shared_from_this が機能しないので無視する
  if (destructed_) {
    return;
  }
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
