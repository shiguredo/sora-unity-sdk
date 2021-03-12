#include "nvcodec_video_decoder.h"

// WebRTC
#include <modules/video_coding/include/video_error_codes.h>
#include <rtc_base/checks.h>
#include <rtc_base/logging.h>
#include <rtc_base/time_utils.h>
#include <third_party/libyuv/include/libyuv/convert.h>

#include "dyn/cuda.h"
#include "dyn/nvcuvid.h"

NvCodecVideoDecoder::NvCodecVideoDecoder(cudaVideoCodec codec_id)
    : codec_id_(codec_id),
      decode_complete_callback_(nullptr),
      buffer_pool_(false, 300 /* max_number_of_buffers*/) {}

NvCodecVideoDecoder::~NvCodecVideoDecoder() {
  Release();
}

void NvCodecVideoDecoder::Log(NvCodecVideoDecoderCuda::LogType type,
                              const std::string& log) {
  if (type == NvCodecVideoDecoderCuda::LogType::LOG_INFO) {
    RTC_LOG(LS_INFO) << log;
  } else if (type == NvCodecVideoDecoderCuda::LogType::LOG_WARNING) {
    RTC_LOG(LS_WARNING) << log;
  } else {
    RTC_LOG(LS_ERROR) << log;
  }
}

bool NvCodecVideoDecoder::IsSupported(cudaVideoCodec codec_id) {
  // CUDA 周りのライブラリがロードできるか確認する
  if (!dyn::DynModule::Instance().IsLoadable(dyn::CUDA_SO)) {
    RTC_LOG(LS_WARNING) << "load library failed: " << dyn::CUDA_SO;
    return false;
  }
  if (!dyn::DynModule::Instance().IsLoadable(dyn::NVCUVID_SO)) {
    RTC_LOG(LS_WARNING) << "load library failed: " << dyn::NVCUVID_SO;
    return false;
  }

  auto decoder =
      NvCodecVideoDecoderCuda::Create(codec_id, &NvCodecVideoDecoder::Log);
  return decoder != nullptr;
}

int32_t NvCodecVideoDecoder::InitDecode(
    const webrtc::VideoCodec* codec_settings,
    int32_t number_of_cores) {
  return InitNvCodec();
}

int32_t NvCodecVideoDecoder::Decode(const webrtc::EncodedImage& input_image,
                                    bool missing_frames,
                                    int64_t render_time_ms) {
  if (decoder_ == nullptr) {
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }
  if (decode_complete_callback_ == nullptr) {
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }
  if (input_image.data() == nullptr && input_image.size() > 0) {
    return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }

  uint8_t** frames = nullptr;
  int frame_count = 0;
  decoder_->Decode(input_image.data(), (int)input_image.size(), frames,
                   frame_count);
  if (frames == nullptr || frame_count == 0) {
    return WEBRTC_VIDEO_CODEC_OK;
  }

  auto nvdec = decoder_->GetNvDecoder();

  // 初回だけデコード情報を出力する
  if (!output_info_) {
    RTC_LOG(LS_INFO) << nvdec->GetVideoInfo();
    output_info_ = true;
  }

  uint32_t pts = input_image.Timestamp();

  // NV12 から I420 に変換
  rtc::scoped_refptr<webrtc::I420Buffer> i420_buffer =
      buffer_pool_.CreateI420Buffer(nvdec->GetWidth(), nvdec->GetHeight());
  libyuv::NV12ToI420(
      frames[0], nvdec->GetDeviceFramePitch(),
      frames[0] + nvdec->GetHeight() * nvdec->GetDeviceFramePitch(),
      nvdec->GetDeviceFramePitch(), i420_buffer->MutableDataY(),
      i420_buffer->StrideY(), i420_buffer->MutableDataU(),
      i420_buffer->StrideU(), i420_buffer->MutableDataV(),
      i420_buffer->StrideV(), nvdec->GetWidth(), nvdec->GetHeight());

  webrtc::VideoFrame decoded_image = webrtc::VideoFrame::Builder()
                                         .set_video_frame_buffer(i420_buffer)
                                         .set_timestamp_rtp(pts)
                                         .build();
  decode_complete_callback_->Decoded(decoded_image, absl::nullopt,
                                     absl::nullopt);

  // 次のフレームで縦横サイズが変わったときに追従するためのマジックコード
  nvdec->setReconfigParams(nullptr, nullptr);

  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t NvCodecVideoDecoder::RegisterDecodeCompleteCallback(
    webrtc::DecodedImageCallback* callback) {
  decode_complete_callback_ = callback;
  return WEBRTC_VIDEO_CODEC_OK;
}

int32_t NvCodecVideoDecoder::Release() {
  ReleaseNvCodec();
  buffer_pool_.Release();
  return WEBRTC_VIDEO_CODEC_OK;
}

bool NvCodecVideoDecoder::PrefersLateDecoding() const {
  return true;
}

const char* NvCodecVideoDecoder::ImplementationName() const {
  return "NVIDIA VIDEO CODEC SDK";
}

int32_t NvCodecVideoDecoder::InitNvCodec() {
  decoder_ =
      NvCodecVideoDecoderCuda::Create(codec_id_, &NvCodecVideoDecoder::Log);
  output_info_ = false;
  return WEBRTC_VIDEO_CODEC_OK;
}

void NvCodecVideoDecoder::ReleaseNvCodec() {
  decoder_.reset();
}
