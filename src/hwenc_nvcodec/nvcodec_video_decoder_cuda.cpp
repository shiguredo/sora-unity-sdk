#include "nvcodec_video_decoder_cuda.h"

#ifdef __cuda_cuda_h__
inline bool check(CUresult e, int iLine, const char* szFile) {
  if (e != CUDA_SUCCESS) {
    const char* szErrName = NULL;
    cuGetErrorName(e, &szErrName);
    std::cerr << "CUDA driver API error " << szErrName << " at line " << iLine
              << " in file " << szFile << std::endl;
    return false;
  }
  return true;
}
#endif

#ifdef __CUDA_RUNTIME_H__
inline bool check(cudaError_t e, int iLine, const char* szFile) {
  if (e != cudaSuccess) {
    std::cerr << "CUDA runtime API error " << cudaGetErrorName(e) << " at line "
              << iLine << " in file " << szFile << std::endl;
    return false;
  }
  return true;
}
#endif

#ifdef _NV_ENCODEAPI_H_
inline bool check(NVENCSTATUS e, int iLine, const char* szFile) {
  const char* aszErrName[] = {
      "NV_ENC_SUCCESS",
      "NV_ENC_ERR_NO_ENCODE_DEVICE",
      "NV_ENC_ERR_UNSUPPORTED_DEVICE",
      "NV_ENC_ERR_INVALID_ENCODERDEVICE",
      "NV_ENC_ERR_INVALID_DEVICE",
      "NV_ENC_ERR_DEVICE_NOT_EXIST",
      "NV_ENC_ERR_INVALID_PTR",
      "NV_ENC_ERR_INVALID_EVENT",
      "NV_ENC_ERR_INVALID_PARAM",
      "NV_ENC_ERR_INVALID_CALL",
      "NV_ENC_ERR_OUT_OF_MEMORY",
      "NV_ENC_ERR_ENCODER_NOT_INITIALIZED",
      "NV_ENC_ERR_UNSUPPORTED_PARAM",
      "NV_ENC_ERR_LOCK_BUSY",
      "NV_ENC_ERR_NOT_ENOUGH_BUFFER",
      "NV_ENC_ERR_INVALID_VERSION",
      "NV_ENC_ERR_MAP_FAILED",
      "NV_ENC_ERR_NEED_MORE_INPUT",
      "NV_ENC_ERR_ENCODER_BUSY",
      "NV_ENC_ERR_EVENT_NOT_REGISTERD",
      "NV_ENC_ERR_GENERIC",
      "NV_ENC_ERR_INCOMPATIBLE_CLIENT_KEY",
      "NV_ENC_ERR_UNIMPLEMENTED",
      "NV_ENC_ERR_RESOURCE_REGISTER_FAILED",
      "NV_ENC_ERR_RESOURCE_NOT_REGISTERED",
      "NV_ENC_ERR_RESOURCE_NOT_MAPPED",
  };
  if (e != NV_ENC_SUCCESS) {
    std::cerr << "NVENC error " << aszErrName[e] << " at line " << iLine
              << " in file " << szFile << std::endl;
    return false;
  }
  return true;
}
#endif

#ifdef _WINERROR_
inline bool check(HRESULT e, int iLine, const char* szFile) {
  if (e != S_OK) {
    std::cerr << "HRESULT error 0x" << (void*)e << " at line " << iLine
              << " in file " << szFile << std::endl;
    return false;
  }
  return true;
}
#endif

#if defined(__gl_h_) || defined(__GL_H__)
inline bool check(GLenum e, int iLine, const char* szFile) {
  if (e != 0) {
    std::cerr << "GLenum error " << e << " at line " << iLine << " in file "
              << szFile << std::endl;
    return false;
  }
  return true;
}
#endif

inline bool check(int e, int iLine, const char* szFile) {
  if (e < 0) {
    std::cerr << "General error " << e << " at line " << iLine << " in file "
              << szFile << std::endl;
    return false;
  }
  return true;
}

#define ck(call) check(call, __LINE__, __FILE__)

std::unique_ptr<NvCodecVideoDecoderCuda> NvCodecVideoDecoderCuda::Create(cudaVideoCodec codec_id) {
  auto p = std::unique_ptr<NvCodecVideoDecoderCuda>(new NvCodecVideoDecoderCuda());
  if (p->Init(codec_id) != 0) {
    return nullptr;
  }
  return p;
}

NvCodecVideoDecoderCuda::~NvCodecVideoDecoderCuda() {
  Release();
}

int32_t NvCodecVideoDecoderCuda::Init(cudaVideoCodec codec_id) {
  ck(cuInit(0));
  ck(cuDeviceGet(&cu_device_, 0));
  char device_name[80];
  ck(cuDeviceGetName(device_name, sizeof(device_name), cu_device_));
  std::cout << "GPU in use: " << device_name << std::endl;
  ck(cuCtxCreate(&cu_context_, 0, cu_device_));

  decoder_.reset(new NvDecoder(cu_context_, false, codec_id));

  return 0;
}

void NvCodecVideoDecoderCuda::Decode(const uint8_t* ptr, int size, uint8_t**& frames, int& frame_count) {
  decoder_->Decode((const uint8_t*)ptr, size, &frames, &frame_count);
}

void NvCodecVideoDecoderCuda::Release() {
  cuCtxDestroy(cu_context_);
}