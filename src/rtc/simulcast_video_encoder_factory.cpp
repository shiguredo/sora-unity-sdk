#include "simulcast_video_encoder_factory.h"

#include <absl/strings/match.h>
#include <media/base/media_constants.h>
#include <media/engine/simulcast_encoder_adapter.h>

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
  // VP8 と H.264 の場合のみ SimulcastEncoderAdapter を挟む
  if (absl::EqualsIgnoreCase(format.name, cricket::kVp8CodecName) ||
      absl::EqualsIgnoreCase(format.name, cricket::kH264CodecName)) {
    return std::unique_ptr<webrtc::VideoEncoder>(
        new webrtc::SimulcastEncoderAdapter(internal_encoder_factory_.get(),
                                            format));
  } else {
    return internal_encoder_factory_->CreateVideoEncoder(format);
  }
}

}  // namespace sora
