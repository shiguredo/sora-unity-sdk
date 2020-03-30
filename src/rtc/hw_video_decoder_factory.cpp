/*
 *  Copyright (c) 2016 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "hw_video_decoder_factory.h"

#include "absl/strings/match.h"
#include "api/video_codecs/sdp_video_format.h"
#include "media/base/codec.h"
#include "media/base/media_constants.h"
#include "modules/video_coding/codecs/h264/include/h264.h"
#include "modules/video_coding/codecs/vp8/include/vp8.h"
#include "modules/video_coding/codecs/vp9/include/vp9.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"

#include "hwenc_nvcodec/nvcodec_video_decoder.h"
#include "h264_format.h"

namespace {

bool IsFormatSupported(
    const std::vector<webrtc::SdpVideoFormat>& supported_formats,
    const webrtc::SdpVideoFormat& format) {
  for (const webrtc::SdpVideoFormat& supported_format : supported_formats) {
    if (cricket::IsSameCodec(format.name, format.parameters,
                             supported_format.name,
                             supported_format.parameters)) {
      return true;
    }
  }
  return false;
}

}  // namespace

namespace sora {

std::vector<webrtc::SdpVideoFormat> HWVideoDecoderFactory::GetSupportedFormats()
    const {
  std::vector<webrtc::SdpVideoFormat> formats;
  formats.push_back(webrtc::SdpVideoFormat(cricket::kVp8CodecName));
  for (const webrtc::SdpVideoFormat& format : webrtc::SupportedVP9Codecs())
    formats.push_back(format);

  std::vector<webrtc::SdpVideoFormat> h264_codecs = {
      CreateH264Format(webrtc::H264::kProfileBaseline, webrtc::H264::kLevel3_1,
                       "1"),
      CreateH264Format(webrtc::H264::kProfileBaseline, webrtc::H264::kLevel3_1,
                       "0"),
      CreateH264Format(webrtc::H264::kProfileConstrainedBaseline,
                       webrtc::H264::kLevel3_1, "1"),
      CreateH264Format(webrtc::H264::kProfileConstrainedBaseline,
                       webrtc::H264::kLevel3_1, "0")};

  for (const webrtc::SdpVideoFormat& h264_format : h264_codecs)
    formats.push_back(h264_format);

  return formats;
}

std::unique_ptr<webrtc::VideoDecoder> HWVideoDecoderFactory::CreateVideoDecoder(
    const webrtc::SdpVideoFormat& format) {
  if (!IsFormatSupported(GetSupportedFormats(), format)) {
    RTC_LOG(LS_ERROR) << "Trying to create decoder for unsupported format";
    return nullptr;
  }

  if (absl::EqualsIgnoreCase(format.name, cricket::kVp8CodecName))
    return webrtc::VP8Decoder::Create();
  if (absl::EqualsIgnoreCase(format.name, cricket::kVp9CodecName))
    return webrtc::VP9Decoder::Create();
  if (absl::EqualsIgnoreCase(format.name, cricket::kH264CodecName))
    return std::unique_ptr<webrtc::VideoDecoder>(
        absl::make_unique<NvCodecVideoDecoder>(cudaVideoCodec_H264));

  RTC_NOTREACHED();
  return nullptr;
}

}