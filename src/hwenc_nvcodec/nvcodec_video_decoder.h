#ifndef NVCODEC_VIDEO_DECODER_H_
#define NVCODEC_VIDEO_DECODER_H_

// WebRTC
#include <api/video_codecs/video_decoder.h>
#include <common_video/include/video_frame_buffer_pool.h>
#include <rtc_base/platform_thread.h>

#include "nvcodec_video_decoder_cuda.h"

class NvCodecVideoDecoder : public webrtc::VideoDecoder {
 public:
  // cudaVideoCodec_H264
  // cudaVideoCodec_VP8
  // cudaVideoCodec_VP9
  NvCodecVideoDecoder(cudaVideoCodec codec_id);
  ~NvCodecVideoDecoder() override;

  static bool IsSupported(cudaVideoCodec codec_id);

  bool Configure(const Settings& settings) override;

  int32_t Decode(const webrtc::EncodedImage& input_image,
                 bool missing_frames,
                 int64_t render_time_ms) override;

  int32_t RegisterDecodeCompleteCallback(
      webrtc::DecodedImageCallback* callback) override;

  int32_t Release() override;

  const char* ImplementationName() const override;

 private:
  static void NvCodecVideoDecoder::Log(NvCodecVideoDecoderCuda::LogType type,
                                       const std::string& log);

  bool InitNvCodec();
  void ReleaseNvCodec();

  webrtc::DecodedImageCallback* decode_complete_callback_ = nullptr;
  webrtc::VideoFrameBufferPool buffer_pool_;

  cudaVideoCodec codec_id_;
  std::unique_ptr<NvCodecVideoDecoderCuda> decoder_;
  bool output_info_ = false;
};

#endif  // NVCODEC_VIDEO_DECODER_H_
