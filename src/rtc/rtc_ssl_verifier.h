#ifndef SORA_RTC_SSL_VERIFIER
#define SORA_RTC_SSL_VERIFIER

// WebRTC
#include <rtc_base/ssl_certificate.h>

namespace sora {

class RTCSSLVerifier : public rtc::SSLCertificateVerifier {
 public:
  RTCSSLVerifier(bool insecure);
  bool Verify(const rtc::SSLCertificate& certificate) override;

 private:
  bool insecure_;
};

}  // namespace sora

#endif