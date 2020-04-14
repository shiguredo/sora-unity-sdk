#ifndef NVCODEC_VIDEO_DECODER_CUDA_H_
#define NVCODEC_VIDEO_DECODER_CUDA_H_

#include <memory>

// NvCodec
#include <NvDecoder/NvDecoder.h>

// cuda
#include <cuda.h>

class NvCodecVideoDecoderCuda {
 public:
  // cudaVideoCodec_H264
  // cudaVideoCodec_VP8
  // cudaVideoCodec_VP9
  static std::unique_ptr<NvCodecVideoDecoderCuda> Create(cudaVideoCodec codec_id);

  NvDecoder* GetNvDecoder() { return decoder_.get(); }
  void Decode(const uint8_t* ptr, int size, uint8_t**& frames, int& frame_count);
  ~NvCodecVideoDecoderCuda();

private:
  int32_t Init(cudaVideoCodec codec_id);
  void Release();

private:
  std::unique_ptr<NvDecoder> decoder_;
  CUdevice cu_device_;
  CUcontext cu_context_;
};

#endif  // NVCODEC_VIDEO_DECODER_CUDA_H_
