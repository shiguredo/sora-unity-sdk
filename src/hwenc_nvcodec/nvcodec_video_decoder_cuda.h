#ifndef NVCODEC_VIDEO_DECODER_CUDA_H_
#define NVCODEC_VIDEO_DECODER_CUDA_H_

#include <memory>
#include <functional>
#include <string>

// NvCodec
#include <NvDecoder/NvDecoder.h>

// cuda
#include <cuda.h>

class NvCodecVideoDecoderCuda {
 public:
  enum class LogType {
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
  };
  // cudaVideoCodec_H264
  // cudaVideoCodec_VP8
  // cudaVideoCodec_VP9
  static std::unique_ptr<NvCodecVideoDecoderCuda> Create(cudaVideoCodec codec_id, std::function<void (LogType, const std::string&)> f);

  NvDecoder* GetNvDecoder() { return decoder_.get(); }
  void Decode(const uint8_t* ptr, int size, uint8_t**& frames, int& frame_count);
  ~NvCodecVideoDecoderCuda();

private:
  int32_t Init(cudaVideoCodec codec_id, const std::function<void (LogType, const std::string&)>& f);
  void Release();

private:
  std::unique_ptr<NvDecoder> decoder_;
  CUdevice cu_device_;
  CUcontext cu_context_;
};

#endif  // NVCODEC_VIDEO_DECODER_CUDA_H_
