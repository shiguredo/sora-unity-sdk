#ifndef SORA_DATA_CHANNEL_ON_ASIO_H_
#define SORA_DATA_CHANNEL_ON_ASIO_H_

// Boost
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/date_time.hpp>

#include "sora_data_channel.h"

namespace sora {

class SoraDataChannelOnAsio : public RTCDataManager {
  struct AsioPoster : SoraDataChannelObserver {
    AsioPoster(boost::asio::io_context& ioc, SoraDataChannelObserver* observer)
        : ioc(ioc), observer(observer) {}
    boost::asio::io_context& ioc;
    SoraDataChannelObserver* observer;
    void OnStateChange(rtc::scoped_refptr<webrtc::DataChannelInterface>
                           data_channel) override {
      boost::asio::post(ioc, [observer = observer, data_channel]() {
        observer->OnStateChange(data_channel);
      });
    }
    void OnMessage(
        rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel,
        const webrtc::DataBuffer& buffer) override {
      boost::asio::post(ioc, [observer = observer, data_channel, buffer]() {
        observer->OnMessage(data_channel, buffer);
      });
    }
  };

 public:
  SoraDataChannelOnAsio(boost::asio::io_context& ioc,
                        SoraDataChannelObserver* observer)
      : ioc_(ioc), poster_(ioc, observer), dc_(&poster_), timer_(ioc) {}
  bool IsOpen(std::string label) const { return dc_.IsOpen(label); }
  void Send(std::string label, const webrtc::DataBuffer& data) {
    dc_.Send(label, data);
  }
  void Close(std::function<void()> on_close) {
    timer_.expires_from_now(boost::posix_time::seconds(10));
    timer_.async_wait([on_close](const boost::system::error_code& ec) {
      if (ec == boost::asio::error::operation_aborted) {
        return;
      }
      on_close();
    });
    dc_.Close([this, on_close]() {
      boost::asio::post(ioc_, [this, on_close]() {
        timer_.cancel();
        on_close();
      });
    });
  }

  void OnDataChannel(
      rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) override {
    dc_.OnDataChannel(data_channel);
  }

 private:
  boost::asio::io_context& ioc_;
  AsioPoster poster_;
  SoraDataChannel dc_;
  boost::asio::deadline_timer timer_;
};

}

#endif