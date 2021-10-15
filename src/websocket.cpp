#include "websocket.h"

#include <utility>

// Boost
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/post.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/websocket/stream.hpp>

// WebRTC
#include <rtc_base/logging.h>

#include "ssl_verifier.h"

namespace sora {

static boost::asio::ssl::context CreateSSLContext() {
  boost::asio::ssl::context ctx(boost::asio::ssl::context::tlsv12);
  //ctx.set_default_verify_paths();
  ctx.set_options(boost::asio::ssl::context::default_workarounds |
                  boost::asio::ssl::context::no_sslv2 |
                  boost::asio::ssl::context::no_sslv3 |
                  boost::asio::ssl::context::single_dh_use);
  return ctx;
}

Websocket::Websocket(boost::asio::io_context& ioc)
    : ws_(new websocket_t(ioc)),
      resolver_(new boost::asio::ip::tcp::resolver(ioc)),
      strand_(ws_->get_executor()),
      close_timeout_timer_(ioc) {
  ws_->write_buffer_bytes(8192);
}
Websocket::Websocket(Websocket::ssl_tag,
                     boost::asio::io_context& ioc,
                     bool insecure)
    : Websocket(Websocket::ssl_tag(), ioc, insecure, CreateSSLContext()) {}
Websocket::Websocket(Websocket::ssl_tag,
                     boost::asio::io_context& ioc,
                     bool insecure,
                     boost::asio::ssl::context ssl_ctx)
    : wss_(new ssl_websocket_t(ioc, ssl_ctx)),
      resolver_(new boost::asio::ip::tcp::resolver(ioc)),
      strand_(wss_->get_executor()),
      close_timeout_timer_(ioc) {
  wss_->write_buffer_bytes(8192);

  wss_->next_layer().set_verify_mode(boost::asio::ssl::verify_peer);
  wss_->next_layer().set_verify_callback(
      [insecure](bool preverified, boost::asio::ssl::verify_context& ctx) {
        if (preverified) {
          return true;
        }
        // insecure の場合は証明書をチェックしない
        if (insecure) {
          return true;
        }
        X509* cert = X509_STORE_CTX_get0_cert(ctx.native_handle());
        STACK_OF(X509)* chain = X509_STORE_CTX_get0_chain(ctx.native_handle());
        return SSLVerifier::VerifyX509(cert, chain);
      });
}
Websocket::Websocket(boost::asio::ip::tcp::socket socket)
    : ws_(new websocket_t(std::move(socket))),
      strand_(ws_->get_executor()),
      close_timeout_timer_(socket.get_executor()) {
  ws_->write_buffer_bytes(8192);
}

Websocket::~Websocket() {
  RTC_LOG(LS_INFO) << "Websocket::~Websocket this=" << (void*)this;
}

bool Websocket::IsSSL() const {
  return wss_ != nullptr;
}

Websocket::websocket_t& Websocket::NativeSocket() {
  return *ws_;
}
Websocket::ssl_websocket_t& Websocket::NativeSecureSocket() {
  return *wss_;
}

void Websocket::Connect(const std::string& url, connect_callback_t on_connect) {
  if (!URLParts::Parse(url, parts_)) {
    on_connect(boost::system::errc::make_error_code(
        boost::system::errc::invalid_argument));
    return;
  }

  // wss と ws のみサポート
  if (parts_.scheme != "wss" && parts_.scheme != "ws") {
    on_connect(boost::system::errc::make_error_code(
        boost::system::errc::invalid_argument));
    return;
  }

  // コンストラクタの設定と接続のスキームが合っているか
  if (IsSSL() && parts_.scheme != "wss" || !IsSSL() && parts_.scheme != "ws") {
    on_connect(boost::system::errc::make_error_code(
        boost::system::errc::invalid_argument));
    return;
  }

  if (IsSSL()) {
    // SNI の設定を行う
    if (!SSL_set_tlsext_host_name(wss_->next_layer().native_handle(),
                                  parts_.host.c_str())) {
      boost::system::error_code ec{static_cast<int>(::ERR_get_error()),
                                   boost::asio::error::get_ssl_category()};
      on_connect(ec);
      return;
    }
  }

  on_connect_ = std::move(on_connect);

  std::string port;
  if (parts_.port.empty()) {
    port = IsSSL() ? "443" : "80";
  } else {
    port = parts_.port;
  }

  // DNS ルックアップ
  resolver_->async_resolve(
      parts_.host, port,
      std::bind(&Websocket::OnResolve, this, std::placeholders::_1,
                std::placeholders::_2));
}

void Websocket::OnResolve(
    boost::system::error_code ec,
    boost::asio::ip::tcp::resolver::results_type results) {
  if (ec) {
    auto on_connect = std::move(on_connect_);
    on_connect(ec);
    return;
  }

  // DNS ルックアップで得られたエンドポイントに対して接続する
  if (IsSSL()) {
    boost::asio::async_connect(
        wss_->next_layer().next_layer(), results.begin(), results.end(),
        std::bind(&Websocket::OnSSLConnect, this, std::placeholders::_1));
  } else {
    boost::asio::async_connect(
        ws_->next_layer(), results.begin(), results.end(),
        std::bind(&Websocket::OnConnect, this, std::placeholders::_1));
  }
}

void Websocket::OnSSLConnect(boost::system::error_code ec) {
  if (ec) {
    auto on_connect = std::move(on_connect_);
    on_connect(ec);
    return;
  }

  // SSL のハンドシェイク
  wss_->next_layer().async_handshake(
      boost::asio::ssl::stream_base::client,
      std::bind(&Websocket::OnSSLHandshake, this, std::placeholders::_1));
}

void Websocket::OnSSLHandshake(boost::system::error_code ec) {
  if (ec) {
    auto on_connect = std::move(on_connect_);
    on_connect(ec);
    return;
  }

  // Websocket のハンドシェイク
  wss_->async_handshake(
      parts_.host, parts_.path_query_fragment,
      std::bind(&Websocket::OnHandshake, this, std::placeholders::_1));
}

void Websocket::OnConnect(boost::system::error_code ec) {
  if (ec) {
    auto on_connect = std::move(on_connect_);
    on_connect(ec);
    return;
  }

  // Websocket のハンドシェイク
  ws_->async_handshake(
      parts_.host, parts_.path_query_fragment,
      std::bind(&Websocket::OnHandshake, this, std::placeholders::_1));
}

void Websocket::OnHandshake(boost::system::error_code ec) {
  auto on_connect = std::move(on_connect_);
  on_connect(ec);
}

void Websocket::Accept(
    boost::beast::http::request<boost::beast::http::string_body> req,
    connect_callback_t on_connect) {
  on_connect_ = std::move(on_connect);
  ws_->async_accept(
      req, std::bind(&Websocket::OnAccept, this, std::placeholders::_1));
}

void Websocket::OnAccept(boost::system::error_code ec) {
  auto on_connect = std::move(on_connect_);
  on_connect(ec);
}

void Websocket::Read(read_callback_t on_read) {
  boost::asio::post(strand_,
                    std::bind(&Websocket::DoRead, this, std::move(on_read)));
}

void Websocket::DoRead(read_callback_t on_read) {
  if (IsSSL()) {
    wss_->async_read(read_buffer_,
                     std::bind(&Websocket::OnRead, this, std::move(on_read),
                               std::placeholders::_1, std::placeholders::_2));
  } else {
    ws_->async_read(read_buffer_,
                    std::bind(&Websocket::OnRead, this, std::move(on_read),
                              std::placeholders::_1, std::placeholders::_2));
  }
}

void Websocket::OnRead(read_callback_t on_read,
                       boost::system::error_code ec,
                       std::size_t bytes_transferred) {
  RTC_LOG(LS_INFO) << "Websocket::OnRead this=" << (void*)this
                   << " ec=" << ec.message();

  if (ec) {
    RTC_LOG(LS_ERROR) << __FUNCTION__ << ": " << ec.message();
  }

  const auto text = boost::beast::buffers_to_string(read_buffer_.data());
  read_buffer_.consume(read_buffer_.size());

  RTC_LOG(LS_INFO) << "Websocket::OnRead this=" << (void*)this
                   << " text=" << text;
  std::move(on_read)(ec, bytes_transferred, std::move(text));
}

void Websocket::WriteText(std::string text, write_callback_t on_write) {
  boost::asio::post(strand_, std::bind(&Websocket::DoWriteText, this,
                                       std::move(text), std::move(on_write)));
}

void Websocket::DoWriteText(std::string text, write_callback_t on_write) {
  bool empty = write_data_.empty();
  boost::beast::flat_buffer buffer;

  const auto n = boost::asio::buffer_copy(buffer.prepare(text.size()),
                                          boost::asio::buffer(text));
  buffer.commit(n);

  write_data_.emplace_back(
      new WriteData{std::move(buffer), std::move(on_write), true});

  if (empty) {
    DoWrite();
  }
}
void Websocket::DoWrite() {
  auto& data = write_data_.front();

  RTC_LOG(LS_INFO) << __FUNCTION__ << ": "
                   << boost::beast::buffers_to_string(data->buffer.data());

  if (IsSSL()) {
    wss_->text(data->text);
    wss_->async_write(data->buffer.data(),
                      std::bind(&Websocket::OnWrite, this,
                                std::placeholders::_1, std::placeholders::_2));
  } else {
    ws_->text(data->text);
    ws_->async_write(data->buffer.data(),
                     std::bind(&Websocket::OnWrite, this, std::placeholders::_1,
                               std::placeholders::_2));
  }
}

void Websocket::OnWrite(boost::system::error_code ec,
                        std::size_t bytes_transferred) {
  RTC_LOG(LS_INFO) << "Websocket::OnWrite this=" << (void*)this
                   << " ec=" << ec.message();

  if (ec) {
    RTC_LOG(LS_ERROR) << __FUNCTION__ << ": " << ec.message();
  }

  auto& data = write_data_.front();
  if (data->callback) {
    std::move(data->callback)(ec, bytes_transferred);
  }

  write_data_.erase(write_data_.begin());

  if (!write_data_.empty()) {
    DoWrite();
  }
}

void Websocket::Close(close_callback_t on_close, int timeout_seconds) {
  boost::asio::post(strand_, std::bind(&Websocket::DoClose, this,
                                       std::move(on_close), timeout_seconds));
}

void Websocket::DoClose(close_callback_t on_close, int timeout_seconds) {
  if (IsSSL()) {
    wss_->async_close(
        boost::beast::websocket::close_code::normal,
        std::bind(&Websocket::OnClose, this, on_close, std::placeholders::_1));
  } else {
    ws_->async_close(
        boost::beast::websocket::close_code::normal,
        std::bind(&Websocket::OnClose, this, on_close, std::placeholders::_1));
  }

  close_timeout_timer_.expires_from_now(
      boost::posix_time::seconds(timeout_seconds));
  close_timeout_timer_.async_wait([on_close, timeout_seconds,
                                   this](boost::system::error_code ec) {
    if (ec) {
      return;
    }
    RTC_LOG(LS_INFO) << "Websocket::Close timeout this=" << (void*)this
                     << " timeout_seconds=" << timeout_seconds;
    closed_ = true;
    on_close(
        boost::system::errc::make_error_code(boost::system::errc::timed_out));
  });
}

void Websocket::OnClose(close_callback_t on_close,
                        boost::system::error_code ec) {
  if (closed_) {
    return;
  }

  RTC_LOG(LS_INFO) << "Websocket::OnClose this=" << (void*)this
                   << " ec=" << ec.message() << " code=" << reason().code
                   << " reason=" << reason().reason;
  close_timeout_timer_.cancel();
  on_close(ec);
}

const boost::beast::websocket::close_reason& Websocket::reason() const {
  return IsSSL() ? wss_->reason() : ws_->reason();
}

}  // namespace sora