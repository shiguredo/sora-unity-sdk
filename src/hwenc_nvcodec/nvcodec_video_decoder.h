#ifndef NVCODEC_VIDEO_DECODER_H_
#define NVCODEC_VIDEO_DECODER_H_

// WebRTC
#include <api/video_codecs/video_decoder.h>
#include <common_video/include/i420_buffer_pool.h>
#include <rtc_base/platform_thread.h>

#include "nvcodec_video_decoder_cuda.h"

class NvCodecVideoDecoder : public webrtc::VideoDecoder {
 public:
  // cudaVideoCodec_H264
  // cudaVideoCodec_VP8
  // cudaVideoCodec_VP9
  NvCodecVideoDecoder(cudaVideoCodec codec_id);
  ~NvCodecVideoDecoder() override;

  int32_t InitDecode(const webrtc::VideoCodec* codec_settings,
                     int32_t number_of_cores) override;

  int32_t Decode(const webrtc::EncodedImage& input_image,
                 bool missing_frames,
                 int64_t render_time_ms) override;

  int32_t RegisterDecodeCompleteCallback(
      webrtc::DecodedImageCallback* callback) override;

  int32_t Release() override;

  // Returns true if the decoder prefer to decode frames late.
  // That is, it can not decode infinite number of frames before the decoded
  // frame is consumed.
  bool PrefersLateDecoding() const override;

  const char* ImplementationName() const override;

 private:
  int32_t InitNvCodec();
  void ReleaseNvCodec();

  int width_ = 0;
  int height_ = 0;
  webrtc::DecodedImageCallback* decode_complete_callback_ = nullptr;
  webrtc::I420BufferPool buffer_pool_;

  cudaVideoCodec codec_id_;
  std::unique_ptr<NvCodecVideoDecoderCuda> decoder_;
  bool output_info_ = false;
};

#endif  // NVCODEC_VIDEO_DECODER_H_
