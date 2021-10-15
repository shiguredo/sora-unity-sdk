#include "sora_signaling.h"
#include "sora_version.h"
#include "zlib_helper.h"

#include <boost/asio/connect.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <boost/json.hpp>
#include <boost/preprocessor/stringize.hpp>

namespace {

const int WS_TIMEOUT_SECONDS = 3;

std::string ToString(
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

std::string ToString(
    webrtc::PeerConnectionInterface::PeerConnectionState state) {
  switch (state) {
    case webrtc::PeerConnectionInterface::PeerConnectionState::kNew:
      return "new";
    case webrtc::PeerConnectionInterface::PeerConnectionState::kConnecting:
      return "connecting";
    case webrtc::PeerConnectionInterface::PeerConnectionState::kConnected:
      return "connected";
    case webrtc::PeerConnectionInterface::PeerConnectionState::kDisconnected:
      return "disconnected";
    case webrtc::PeerConnectionInterface::PeerConnectionState::kFailed:
      return "failed";
    case webrtc::PeerConnectionInterface::PeerConnectionState::kClosed:
      return "closed";
  }
  return "unknown";
}

}  // namespace

namespace sora {

std::shared_ptr<SoraSignaling> SoraSignaling::Create(
    boost::asio::io_context& ioc,
    RTCManager* manager,
    SoraSignalingConfig config) {
  return std::shared_ptr<SoraSignaling>(
      new SoraSignaling(ioc, manager, std::move(config)));
}

SoraSignaling::SoraSignaling(boost::asio::io_context& ioc,
                             RTCManager* manager,
                             SoraSignalingConfig config)
    : ioc_(ioc),
      manager_(manager),
      config_(std::move(config)),
      connection_timeout_timer_(ioc),
      closing_timeout_timer_(ioc) {}

void SoraSignaling::Destroy() {
  if (destroy_) {
    return;
  }
  destroy_ = true;

  RTC_LOG(LS_INFO) << "SoraSignaling::Destroy started";

  {
    std::shared_ptr<std::promise<void>> promise(new std::promise<void>());
    std::future<void> future = promise->get_future();
    // DC が有効な場合、一応閉じる努力をする
    if (state_ == State::Connected && using_datachannel_) {
      state_ = State::Closing;

      webrtc::DataBuffer disconnect = ConvertToDataBuffer(
          "signaling", R"({"type":"disconnect","reason":"INTERNAL-ERROR"})");
      dc_->Close(
          disconnect,
          [promise](boost::system::error_code) { promise->set_value(); }, 0.3);
    } else {
      promise->set_value();
    }

    future.wait_for(std::chrono::milliseconds(500));
  }

  // 後始末
  auto connection = connection_;
  connection_ = nullptr;
  // ここで OnIceConnectionStateChange などが呼ばれる
  connection = nullptr;
  ws_ = nullptr;
  ws_connected_ = false;
  dc_ = nullptr;
  using_datachannel_ = false;
  state_ = State::Closed;

  RTC_LOG(LS_INFO) << "SoraSignaling::Destroy completed";
}

SoraSignaling::~SoraSignaling() {
  Destroy();
}

void SoraSignaling::Post(std::function<void()> f) {
  if (destroy_) {
    return;
  }
  boost::asio::post(ioc_, f);
}

std::shared_ptr<RTCConnection> SoraSignaling::GetRTCConnection() const {
  std::promise<std::shared_ptr<RTCConnection>> promise;
  std::future<std::shared_ptr<RTCConnection>> future = promise.get_future();
  boost::asio::dispatch(ioc_, [this, promise = std::move(promise)]() mutable {
    promise.set_value(connection_);
  });
  auto status = future.wait_for(std::chrono::milliseconds(100));
  if (status != std::future_status::ready) {
    return nullptr;
  }
  return future.get();
}

void SoraSignaling::Connect() {
  RTC_LOG(LS_INFO) << "SoraSignaling::Connect";

  Post([self = shared_from_this()]() { self->DoConnect(); });
}

void SoraSignaling::DoConnect() {
  if (state_ != State::Init && state_ != State::Closed) {
    return;
  }

  dc_.reset(new SoraDataChannelOnAsio(ioc_, this));
  manager_->SetDataManager(dc_);

  // 接続タイムアウト用の処理
  connection_timeout_timer_.expires_from_now(boost::posix_time::seconds(30));
  connection_timeout_timer_.async_wait(
      [self = shared_from_this()](boost::system::error_code ec) {
        if (ec) {
          return;
        }

        RTC_LOG(LS_ERROR) << "Connection timeout";
        self->state_ = State::Closed;
        self->config_.on_disconnect((int)sora_conf::ErrorCode::INTERNAL_ERROR,
                                    "Connection timeout");
      });

  std::string error_messages;
  for (const auto& url : config_.signaling_url) {
    URLParts parts;
    if (!URLParts::Parse(url, parts)) {
      RTC_LOG(LS_WARNING) << "Invalid Signaling URL: " << url;
      error_messages += "Invalid Signaling URL: " + url + " | ";
      continue;
    }

    std::shared_ptr<Websocket> ws;
    if (parts.scheme == "ws") {
      ws.reset(new Websocket(ioc_));
    } else if (parts.scheme == "wss") {
      ws.reset(new Websocket(Websocket::ssl_tag(), ioc_, config_.insecure));
    } else {
      error_messages += "Invalid Scheme: " + parts.scheme + " | ";
      continue;
    }

    ws->Connect(url, std::bind(&SoraSignaling::OnConnect, shared_from_this(),
                               std::placeholders::_1, url, ws));
    connecting_wss_.push_back(ws);
  }
  if (connecting_wss_.empty()) {
    state_ = State::Closed;
    config_.on_disconnect((int)sora_conf::ErrorCode::INVALID_PARAMETER,
                          error_messages);
    return;
  }

  state_ = State::Connecting;
}

void SoraSignaling::OnConnect(boost::system::error_code ec,
                              std::string url,
                              std::shared_ptr<Websocket> ws) {
  RTC_LOG(LS_INFO) << __FUNCTION__ << " url=" << url;

  assert(state_ == State::Connecting || state_ == State::Redirecting ||
         state_ == State::Connected || state_ == State::Closed);

  connecting_wss_.erase(
      std::remove_if(connecting_wss_.begin(), connecting_wss_.end(),
                     [ws](std::shared_ptr<Websocket> p) { return p == ws; }),
      connecting_wss_.end());

  if (state_ == State::Closed) {
    return;
  }

  if (ec) {
    RTC_LOG(LS_WARNING) << "Failed Websocket handshake: " << ec;
    // すべての接続がうまくいかなかったら終了する
    if (state_ == State::Connecting && connecting_wss_.empty()) {
      state_ = State::Closed;
      config_.on_disconnect(
          (int)sora_conf::ErrorCode::WEBSOCKET_HANDSHAKE_FAILED,
          "Failed Websocket handshake: last_ec=" + ec.message() +
              " last_url=" + url);
    }
    return;
  }

  if (state_ == State::Connected || state_ == State::Redirecting) {
    // 既に他の接続が先に完了していたので、切断する
    ws->Close([self = shared_from_this(), ws](boost::system::error_code) {},
              WS_TIMEOUT_SECONDS);
    return;
  }

  connection_timeout_timer_.cancel();

  RTC_LOG(LS_INFO) << "Signaling Websocket is connected: url=" << url;
  state_ = State::Connected;
  ws_ = ws;
  ws_connected_ = true;
  connected_signaling_url_ = url;

  DoRead();
  DoSendConnect(false);
}

void SoraSignaling::Redirect(std::string url) {
  assert(state_ == State::Connected);

  state_ = State::Redirecting;

  ws_->Read([self = shared_from_this(), url](boost::system::error_code ec,
                                             std::size_t bytes_transferred,
                                             std::string text) {
    // リダイレクト中に Disconnect が呼ばれた
    if (self->state_ != State::Redirecting) {
      return;
    }

    auto on_close = [self, url](boost::system::error_code ec) {
      if (self->state_ != State::Redirecting) {
        return;
      }

      // close 処理に成功してても失敗してても処理は続ける
      if (ec) {
        RTC_LOG(LS_WARNING) << "Redirect error: ec=" << ec.message();
      }

      // 接続タイムアウト用の処理
      self->connection_timeout_timer_.expires_from_now(
          boost::posix_time::seconds(30));
      self->connection_timeout_timer_.async_wait(
          [self](boost::system::error_code ec) {
            if (ec) {
              return;
            }

            RTC_LOG(LS_ERROR) << "Connection timeout in redirection";
            self->state_ = State::Closed;
            self->config_.on_disconnect(
                (int)sora_conf::ErrorCode::INTERNAL_ERROR,
                "Connection timeout in redirection");
          });

      URLParts parts;
      if (!URLParts::Parse(url, parts)) {
        RTC_LOG(LS_ERROR) << "Invalid URL: url=" << url;
        self->config_.on_disconnect(
            (int)sora_conf::ErrorCode::INVALID_PARAMETER,
            "Invalid URL: url=" + url);
        self->state_ = State::Closed;
        return;
      }

      std::shared_ptr<Websocket> ws;
      if (parts.scheme == "wss") {
        ws.reset(new Websocket(Websocket::ssl_tag(), self->ioc_,
                               self->config_.insecure));
      } else if (parts.scheme == "ws") {
        ws.reset(new Websocket(self->ioc_));
      } else {
        RTC_LOG(LS_ERROR) << "Invalid Scheme: url=" << url;
        self->config_.on_disconnect(
            (int)sora_conf::ErrorCode::INVALID_PARAMETER,
            "Invalid Scheme: url=" + url);
        return;
      }

      ws->Connect(url, std::bind(&SoraSignaling::OnRedirect, self,
                                 std::placeholders::_1, url, ws));
    };

    // type: redirect の後、サーバは切断してるはずなので、正常に処理が終わるのはおかしい
    if (!ec) {
      RTC_LOG(LS_WARNING) << "Unexpected success to read";
      // 強制的に閉じる
      self->ws_->Close(on_close, WS_TIMEOUT_SECONDS);
      return;
    }
    on_close(
        boost::system::errc::make_error_code(boost::system::errc::success));
  });
}

void SoraSignaling::OnRedirect(boost::system::error_code ec,
                               std::string url,
                               std::shared_ptr<Websocket> ws) {
  if (state_ != State::Redirecting) {
    return;
  }

  if (ec) {
    RTC_LOG(LS_ERROR) << "Failed Websocket handshake: " << ec;
    state_ = State::Closed;
    config_.on_disconnect((int)sora_conf::ErrorCode::WEBSOCKET_HANDSHAKE_FAILED,
                          "Failed Websocket handshake in redirecting: ec=" +
                              ec.message() + " url=" + url);
    return;
  }

  connection_timeout_timer_.cancel();

  state_ = State::Connected;
  ws_ = ws;
  ws_connected_ = true;
  connected_signaling_url_ = url;
  RTC_LOG(LS_INFO) << "redirected: url=" << url;

  DoRead();
  DoSendConnect(true);
}

#define SORA_CLIENT \
  "Sora Unity SDK " SORA_UNITY_SDK_VERSION " (" SORA_UNITY_SDK_COMMIT_SHORT ")"
#define LIBWEBRTC                                                      \
  "Shiguredo-build " WEBRTC_READABLE_VERSION " (" WEBRTC_BUILD_VERSION \
  " " WEBRTC_SRC_COMMIT_SHORT ")"

void SoraSignaling::DoSendConnect(bool redirect) {
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

  if (redirect) {
    json_message["redirect"] = true;
  }

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

  ws_->WriteText(
      boost::json::serialize(json_message),
      [self = shared_from_this()](boost::system::error_code, size_t) {});
}
void SoraSignaling::DoSendPong() {
  boost::json::value json_message = {{"type", "pong"}};
  ws_->WriteText(
      boost::json::serialize(json_message),
      [self = shared_from_this()](boost::system::error_code, size_t) {});
}
void SoraSignaling::DoSendPong(
    const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report) {
  std::string stats = report->ToJson();
  if (using_datachannel_ && dc_->IsOpen("stats")) {
    // DataChannel が使える場合は type: stats で DataChannel に送る
    std::string str = R"({"type":"stats","reports":)" + stats + "}";
    SendDataChannel("stats", str);
  } else if (ws_connected_) {
    std::string str = R"({"type":"pong","stats":)" + stats + "}";
    ws_->WriteText(std::move(str), [self = shared_from_this()](
                                       boost::system::error_code, size_t) {});
  }
}
void SoraSignaling::DoSendUpdate(const std::string& sdp, std::string type) {
  boost::json::value json_message = {{"type", type}, {"sdp", sdp}};
  if (using_datachannel_ && dc_->IsOpen("signaling")) {
    // DataChannel が使える場合は DataChannel に送る
    SendDataChannel("signaling", boost::json::serialize(json_message));
  } else if (ws_connected_) {
    ws_->WriteText(
        boost::json::serialize(json_message),
        [self = shared_from_this()](boost::system::error_code, size_t) {});
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

void SoraSignaling::Disconnect() {
  Post([self = shared_from_this()]() { self->DoDisconnect(); });
}
void SoraSignaling::DoDisconnect() {
  if (state_ == State::Init) {
    state_ = State::Closed;
    return;
  }
  if (state_ == State::Connecting) {
    connection_timeout_timer_.cancel();
    config_.on_disconnect((int)sora_conf::ErrorCode::CLOSE_SUCCEEDED,
                          "Close was called in connecting");
    state_ = State::Closed;
    return;
  }
  if (state_ == State::Closing) {
    return;
  }
  if (state_ == State::Closed) {
    return;
  }

  DoInternalDisconnect(boost::none, "", "");
}

void SoraSignaling::SendMessage(const std::string& label,
                                const std::string& data) {
  Post([self = shared_from_this(), label, data]() {
    self->SendDataChannel(label, data);
  });
}

void SoraSignaling::DoRead() {
  ws_->Read([self = shared_from_this()](boost::system::error_code ec,
                                        std::size_t bytes_transferred,
                                        std::string text) {
    self->OnRead(ec, bytes_transferred, std::move(text));
  });
}

void SoraSignaling::OnRead(boost::system::error_code ec,
                           std::size_t bytes_transferred,
                           std::string text) {
  boost::ignore_unused(bytes_transferred);

  if (ec) {
    assert(state_ == State::Connected || state_ == State::Closing ||
           state_ == State::Closed);
    if (state_ == State::Closed) {
      // 全て終わってるので何もしない
      return;
    }
    if (state_ == State::Closing) {
      if (on_ws_close_) {
        // Close で切断されて呼ばれたコールバックなので、on_ws_close_ を呼んで続きを処理してもらう
        on_ws_close_(ec);
      }
      return;
    }
    if (state_ == State::Connected && !using_datachannel_) {
      // 何かエラーが起きたので切断する
      ws_connected_ = false;
      state_ = State::Closed;
      auto error_code = ec == boost::beast::websocket::error::closed
                            ? (int)sora_conf::ErrorCode::WEBSOCKET_ONCLOSE
                            : (int)sora_conf::ErrorCode::WEBSOCKET_ONERROR;
      auto reason = ws_->reason();
      std::string reason_str = " wscode=" + std::to_string(reason.code) +
                               " wsreason=" + reason.reason.c_str();
      config_.on_disconnect(error_code, "Failed to read WebSocket: ec=" +
                                            ec.message() + reason_str);
      RTC_LOG(LS_ERROR) << "Failed to read: ec=" << ec.message() + reason_str;
      return;
    }
    if (state_ == State::Connected && using_datachannel_ && !ws_connected_) {
      // ignore_disconnect_websocket による WS 切断なので何もしない
      return;
    }
    if (state_ == State::Connected && using_datachannel_ && ws_connected_) {
      // DataChanel で reason: "WEBSOCKET-ONCLOSE" または "WEBSOCKET-ONERROR" を送る事を試みてから終了する
      webrtc::DataBuffer disconnect = ConvertToDataBuffer(
          "signaling",
          ec == boost::beast::websocket::error::closed
              ? R"({"type":"disconnect","reason":"WEBSOCKET-ONCLOSE"})"
              : R"({"type":"disconnect","reason":"WEBSOCKET-ONERROR"})");
      auto error_code = ec == boost::beast::websocket::error::closed
                            ? (int)sora_conf::ErrorCode::WEBSOCKET_ONCLOSE
                            : (int)sora_conf::ErrorCode::WEBSOCKET_ONERROR;
      auto reason = ws_->reason();
      std::string reason_str = " wscode=" + std::to_string(reason.code) +
                               " wsreason=" + reason.reason.c_str();
      state_ = State::Closing;
      dc_->Close(
          disconnect,
          [self = shared_from_this(), ec, error_code,
           reason_str](boost::system::error_code ec2) {
            self->ws_connected_ = false;
            self->using_datachannel_ = false;
            self->state_ = State::Closed;

            if (ec2) {
              self->config_.on_disconnect(
                  error_code,
                  "Failed to read WebSocket and to close DataChannel: ec=" +
                      ec.message() + " ec2=" + ec2.message() + reason_str);
              return;
            }
            self->config_.on_disconnect(
                error_code,
                "Failed to read WebSocket and succeeded to close "
                "DataChannel: ec=" +
                    ec.message() + reason_str);
          },
          config_.disconnect_wait_timeout);
      return;
    }

    // ここに来ることは無いはず
    ws_connected_ = false;
    state_ = State::Closed;
    config_.on_disconnect((int)sora_conf::ErrorCode::INTERNAL_ERROR,
                          "Failed to read WebSocket: ec=" + ec.message());
    RTC_LOG(LS_ERROR) << "Failed to read: ec=" << ec.message();
    return;
  }

  if (state_ != State::Connected) {
    return;
  }

  RTC_LOG(LS_INFO) << __FUNCTION__ << ": text=" << text;

  auto json_message = boost::json::parse(text);
  const std::string type = json_message.at("type").as_string().c_str();

  // connection_ が初期化される前に offer, redirect 以外がやってきたら単に無視する
  if (type != "offer" && type != "redirect" && connection_ == nullptr) {
    DoRead();
    return;
  }

  if (type == "redirect") {
    const std::string location =
        json_message.at("location").as_string().c_str();
    Redirect(location);
    // Redirect の中で次の Read をしているのでここで return する
    return;
  } else if (type == "offer") {
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

    connection_->SetOffer(
        sdp,
        [self = shared_from_this(), json_message]() {
          self->Post([self, json_message]() {
            if (self->state_ != State::Connected) {
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

              // TODO(melpon): しばらく mid が無い可能性も考慮するが、そのうち必須にする
              std::string mid;
              {
                auto it = json_message.as_object().find("mid");
                if (it != json_message.as_object().end()) {
                  const auto& midobj = it->value().as_object();
                  // video: false の場合は video フィールドが mid が無いのでチェックする
                  it = midobj.find("video");
                  if (it != midobj.end()) {
                    mid = it->value().as_string().c_str();
                  }
                }
              }
              self->connection_->SetEncodingParameters(
                  mid, std::move(encoding_parameters));
            }

            self->connection_->CreateAnswer(
                [self](webrtc::SessionDescriptionInterface* desc) {
                  std::string sdp;
                  desc->ToString(&sdp);
                  self->Post([self, sdp]() {
                    if (self->state_ != State::Connected) {
                      return;
                    }
                    boost::json::value json_message = {{"type", "answer"},
                                                       {"sdp", sdp}};
                    self->ws_->WriteText(
                        boost::json::serialize(json_message),
                        [self](boost::system::error_code, size_t) {});
                  });
                },
                self->CreateIceError(
                    "Failed to CreateAnswer in offer message via WebSocket"));
          });
        },
        CreateIceError("Failed to SetOffer in offer message via WebSocket"));
  } else if (type == "update" || type == "re-offer") {
    std::string answer_type = type == "update" ? "update" : "re-answer";
    const std::string sdp = json_message.at("sdp").as_string().c_str();
    connection_->SetOffer(
        sdp,
        [self = shared_from_this(), type, answer_type]() {
          self->Post([self, type, answer_type]() {
            if (self->state_ != State::Connected) {
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
                  self->Post([self, sdp, answer_type]() {
                    if (self->state_ != State::Connected) {
                      return;
                    }

                    self->DoSendUpdate(sdp, answer_type);
                  });
                },
                self->CreateIceError("Failed to CreateAnswer in " + type +
                                     " message via WebSocket"));
          });
        },
        CreateIceError("Failed to SetOffer in " + type +
                       " message via WebSocket"));
  } else if (type == "notify") {
    if (config_.on_notify) {
      config_.on_notify(text);
    }
  } else if (type == "push") {
    if (config_.on_push) {
      config_.on_push(text);
    }
  } else if (type == "ping") {
    auto it = json_message.as_object().find("stats");
    if (it != json_message.as_object().end() && it->value().as_bool()) {
      connection_->GetStats(
          [self = shared_from_this()](
              const rtc::scoped_refptr<const webrtc::RTCStatsReport>& report) {
            if (self->state_ != State::Connected) {
              return;
            }

            self->DoSendPong(report);
          });
    } else {
      DoSendPong();
    }
  } else if (type == "switched") {
    // Data Channel による通信の開始
    using_datachannel_ = true;

    // ignore_disconnect_websocket == true の場合は WS を切断する
    auto it = json_message.as_object().find("ignore_disconnect_websocket");
    if (it != json_message.as_object().end() && it->value().as_bool() &&
        ws_connected_) {
      RTC_LOG(LS_INFO) << "Close WebSocket for DataChannel";
      ws_->Close([self = shared_from_this()](boost::system::error_code) {},
                 WS_TIMEOUT_SECONDS);
      ws_connected_ = false;
      return;
    }
  }

  DoRead();
}

// 出来るだけサーバに type: disconnect を送ってから閉じる
// force_error_code が設定されていない場合、NO-ERROR でサーバに送信し、ユーザにはデフォルトのエラーコードとメッセージでコールバックする。reason や message は無視される。
// force_error_code が設定されている場合、reason でサーバに送信し、指定されたエラーコードと、message にデフォルトのメッセージを追加したメッセージでコールバックする。
void SoraSignaling::DoInternalDisconnect(boost::optional<int> force_error_code,
                                         std::string reason,
                                         std::string message) {
  assert(state_ == State::Connected);

  state_ = State::Closing;

  auto on_close = [self = shared_from_this(), force_error_code,
                   root_message = message](bool succeeded, int error_code,
                                           std::string message) {
    if (!succeeded) {
      RTC_LOG(LS_ERROR) << message;
    }
    self->connection_ = nullptr;
    self->using_datachannel_ = false;
    self->ws_connected_ = false;
    self->on_ws_close_ = nullptr;
    self->state_ = State::Closed;

    if (force_error_code == boost::none) {
      self->config_.on_disconnect(error_code, message);
    } else {
      self->config_.on_disconnect(
          *force_error_code, root_message + ", internal_message=" + message);
    }
  };

  if (using_datachannel_ && ws_connected_) {
    webrtc::DataBuffer disconnect = ConvertToDataBuffer(
        "signaling",
        force_error_code == boost::none
            ? R"({"type":"disconnect","reason":"NO-ERROR"})"
            : R"({"type":"disconnect","reason":")" + reason + "\"}");
    dc_->Close(
        disconnect,
        [self = shared_from_this(), on_close, force_error_code,
         message](boost::system::error_code ec1) {
          self->closing_timeout_timer_.expires_from_now(
              boost::posix_time::seconds(3));
          self->closing_timeout_timer_.async_wait(
              [self](boost::system::error_code ec) {
                if (ec) {
                  return;
                }
                self->on_ws_close_(boost::system::errc::make_error_code(
                    boost::system::errc::timed_out));
              });
          self->on_ws_close_ = [self, ec1,
                                on_close](boost::system::error_code ec2) {
            self->closing_timeout_timer_.cancel();
            auto ws_reason = self->ws_->reason();
            std::string ws_reason_str =
                " wscode=" + std::to_string(ws_reason.code) +
                " wsreason=" + ws_reason.reason.c_str();

            bool ec2_error = ec2 != boost::beast::websocket::error::closed;
            bool succeeded = true;
            std::string message =
                "Succeeded to close DataChannel and Websocket";
            int error_code = (int)sora_conf::ErrorCode::CLOSE_SUCCEEDED;
            if (ec1 && ec2_error) {
              succeeded = false;
              message = "Failed to close DataChannel and WebSocket: ec1=" +
                        ec1.message() + " ec2=" + ec2.message() + ws_reason_str;
              error_code = (int)sora_conf::ErrorCode::CLOSE_FAILED;
            } else if (ec1 && !ec2_error) {
              succeeded = false;
              message = "Failed to close DataChannel (WS succeeded): ec=" +
                        ec1.message() + ws_reason_str;
              error_code = (int)sora_conf::ErrorCode::CLOSE_FAILED;
            } else if (!ec1 && ec2_error) {
              succeeded = false;
              message = "Failed to close WebSocket (DC succeeded): ec=" +
                        ec2.message() + ws_reason_str;
              error_code = (int)sora_conf::ErrorCode::CLOSE_FAILED;
            }
            on_close(succeeded, error_code, message);
          };
        },
        config_.disconnect_wait_timeout);
  } else if (using_datachannel_ && !ws_connected_) {
    webrtc::DataBuffer disconnect = ConvertToDataBuffer(
        "signaling", R"({"type":"disconnect","reason":"NO-ERROR"})");
    dc_->Close(
        disconnect,
        [on_close](boost::system::error_code ec) {
          if (ec) {
            on_close(false, (int)sora_conf::ErrorCode::CLOSE_FAILED,
                     "Failed to close DataChannel: ec=" + ec.message());
            return;
          }
          on_close(true, (int)sora_conf::ErrorCode::CLOSE_SUCCEEDED,
                   "Succeeded to close DataChannel");
        },
        config_.disconnect_wait_timeout);
  } else if (!using_datachannel_ && ws_connected_) {
    boost::json::value disconnect = {{"type", "disconnect"},
                                     {"reason", "NO-ERROR"}};
    ws_->WriteText(
        boost::json::serialize(disconnect),
        [self = shared_from_this(), on_close](boost::system::error_code ec,
                                              std::size_t) {
          if (ec) {
            on_close(
                false, (int)sora_conf::ErrorCode::CLOSE_FAILED,
                "Failed to write disconnect message to close WebSocket: ec=" +
                    ec.message());
            return;
          }

          self->closing_timeout_timer_.expires_from_now(
              boost::posix_time::seconds(3));
          self->closing_timeout_timer_.async_wait(
              [self](boost::system::error_code ec) {
                if (ec) {
                  return;
                }
                self->on_ws_close_(boost::system::errc::make_error_code(
                    boost::system::errc::timed_out));
              });
          self->on_ws_close_ = [self, on_close](boost::system::error_code ec) {
            self->closing_timeout_timer_.cancel();
            bool ec_error = ec != boost::beast::websocket::error::closed;
            if (ec_error) {
              auto reason = self->ws_->reason();
              on_close(false, (int)sora_conf::ErrorCode::CLOSE_FAILED,
                       "Failed to close WebSocket: ec=" + ec.message() +
                           " wscode=" + std::to_string(reason.code) +
                           " wsreason=" + reason.reason.c_str());
              return;
            }
            on_close(true, (int)sora_conf::ErrorCode::CLOSE_SUCCEEDED,
                     "Succeeded to close WebSocket");
          };
        });
  } else {
    on_close(false, (int)sora_conf::ErrorCode::INTERNAL_ERROR,
             "Unknown state. WS and DC is already released.");
  }
}

std::function<void(webrtc::RTCError)> SoraSignaling::CreateIceError(
    std::string message) {
  return [self = shared_from_this(), message](webrtc::RTCError error) {
    self->Post([self, message, error]() {
      if (self->state_ != State::Connected) {
        return;
      }
      self->DoInternalDisconnect((int)sora_conf::ErrorCode::ICE_FAILED,
                                 "INTERNAL-ERROR",
                                 message + ": error=" + error.message());
    });
  };
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
    if (config_.on_message) {
      config_.on_message(label, data);
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
      connection_->SetOffer(
          sdp,
          [self = shared_from_this()]() {
            self->Post([self]() {
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
                    self->Post([self, sdp]() {
                      if (!self->connection_) {
                        return;
                      }
                      self->DoSendUpdate(sdp, "re-answer");
                    });
                  },
                  self->CreateIceError("Failed to CreateAnswer in re-offer "
                                       "message via WebSocket"));
            });
          },
          CreateIceError(
              "Failed to SetOffer in re-offer message via WebSocket"));
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
    if (config_.on_notify) {
      config_.on_notify(data);
    }
  }

  if (label == "push") {
    if (config_.on_push) {
      config_.on_push(data);
    }
  }
}

// WebRTC からのコールバック
// これらは別スレッドからやってくるので取り扱い注意
void SoraSignaling::OnIceConnectionStateChange(
    webrtc::PeerConnectionInterface::IceConnectionState new_state) {
  if (!connection_) {
    return;
  }
  Post([self = shared_from_this(), new_state]() {
    RTC_LOG(LS_INFO) << "IceConnectionStateChange: ["
                     << ToString(self->ice_state_) << "]->["
                     << ToString(new_state) << "]";
    self->ice_state_ = new_state;
  });
}
void SoraSignaling::OnConnectionChange(
    webrtc::PeerConnectionInterface::PeerConnectionState new_state) {
  if (!connection_) {
    return;
  }
  Post([self = shared_from_this(), new_state]() {
    self->DoConnectionChange(new_state);
  });
}
void SoraSignaling::OnIceCandidate(const std::string sdp_mid,
                                   const int sdp_mlineindex,
                                   const std::string sdp) {
  boost::json::value json_message = {{"type", "candidate"}, {"candidate", sdp}};
  Post([self = shared_from_this(), json_message]() {
    if (self->state_ != State::Connected) {
      return;
    }

    self->ws_->WriteText(boost::json::serialize(json_message),
                         [self](boost::system::error_code, size_t) {});
  });
}
void SoraSignaling::DoConnectionChange(
    webrtc::PeerConnectionInterface::PeerConnectionState new_state) {
  RTC_LOG(LS_INFO) << "ConnectionChange: [" << ToString(connection_state_)
                   << "]->[" << ToString(new_state) << "]";
  connection_state_ = new_state;

  // Failed になったら諦めるしか無いので終了処理に入る
  if (new_state ==
      webrtc::PeerConnectionInterface::PeerConnectionState::kFailed) {
    if (state_ != State::Connected) {
      // この場合別のフローから終了処理に入ってるはずなので無視する
      return;
    }
    // disconnect を送る必要は無い（そもそも failed になってるということは届かない）のでそのまま落とすだけ
    connection_ = nullptr;
    using_datachannel_ = false;
    ws_connected_ = false;
    state_ = State::Closed;
    config_.on_disconnect(
        (int)sora_conf::ErrorCode::PEER_CONNECTION_STATE_FAILED,
        "PeerConnectionState::kFailed");
  }
}

}  // namespace sora