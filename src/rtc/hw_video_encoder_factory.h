#ifndef HW_VIDEO_ENCODER_FACTORY_H_
#define HW_VIDEO_ENCODER_FACTORY_H_

#include <memory>
#include <vector>

#ifdef _WIN32
#include <d3d11.h>
#endif

#include "api/video_codecs/sdp_video_format.h"
#include "api/video_codecs/video_encoder.h"
#include "api/video_codecs/video_encoder_factory.h"

namespace sora {

class HWVideoEncoderFactory : public webrtc::VideoEncoderFactory {
 public:
  HWVideoEncoderFactory(bool simulcast);
  virtual ~HWVideoEncoderFactory() {}

  std::vector<webrtc::SdpVideoFormat> GetSupportedFormats() const override;

  std::unique_ptr<webrtc::VideoEncoder> CreateVideoEncoder(
      const webrtc::SdpVideoFormat& format) override;

  std::unique_ptr<webrtc::VideoEncoder> HWVideoEncoderFactory::WithSimulcast(
      const webrtc::SdpVideoFormat& format,
      std::function<std::unique_ptr<webrtc::VideoEncoder>(
          const webrtc::SdpVideoFormat&)> create);

 private:
  std::unique_ptr<HWVideoEncoderFactory> internal_encoder_factory_;
};

}  // namespace sora

#endif  // HW_VIDEO_ENCODER_FACTORY_H_
