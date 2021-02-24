#include "simulcast_video_encoder_factory.h"

#include <absl/strings/match.h>
#include <media/engine/simulcast_encoder_adapter.h>
#include <modules/video_coding/codecs/vp9/include/vp9.h>

namespace sora {

SimulcastVideoEncoderFactory::SimulcastVideoEncoderFactory(
    std::unique_ptr<webrtc::VideoEncoderFactory> internal_encoder_factory)
    : internal_encoder_factory_(std::move(internal_encoder_factory)) {}

std::vector<webrtc::SdpVideoFormat>
SimulcastVideoEncoderFactory::GetSupportedFormats() const {
  return internal_encoder_factory_->GetSupportedFormats();
}

std::unique_ptr<webrtc::VideoEncoder>
SimulcastVideoEncoderFactory::CreateVideoEncoder(
    const webrtc::SdpVideoFormat& format) {
  // VP9 の場合は SimulcastEncoderAdapter を挟まない
  if (absl::EqualsIgnoreCase(format.name, cricket::kVp9CodecName)) {
    return internal_encoder_factory_->CreateVideoEncoder(format);
  }

  return std::unique_ptr<webrtc::VideoEncoder>(
      new webrtc::SimulcastEncoderAdapter(internal_encoder_factory_.get(),
                                          format));
}

}  // namespace sora
