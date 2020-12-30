#ifndef SIMULCAST_VIDEO_ENCODER_FACTORY_H_
#define SIMULCAST_VIDEO_ENCODER_FACTORY_H_

#include <memory>
#include <vector>

// WebRTC
#include <api/video_codecs/sdp_video_format.h>
#include <api/video_codecs/video_encoder.h>
#include <api/video_codecs/video_encoder_factory.h>

namespace sora {

class SimulcastVideoEncoderFactory : public webrtc::VideoEncoderFactory {
 public:
  SimulcastVideoEncoderFactory(
      std::unique_ptr<webrtc::VideoEncoderFactory> internal_encoder_factory);
  virtual ~SimulcastVideoEncoderFactory() {}

  std::vector<webrtc::SdpVideoFormat> GetSupportedFormats() const override;

  std::unique_ptr<webrtc::VideoEncoder> CreateVideoEncoder(
      const webrtc::SdpVideoFormat& format) override;

 private:
  std::unique_ptr<webrtc::VideoEncoderFactory> internal_encoder_factory_;
};

}  // namespace sora

#endif
