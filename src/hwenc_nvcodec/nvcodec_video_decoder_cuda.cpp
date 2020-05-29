#include "nvcodec_video_decoder_cuda.h"

#include <utility>
#include <sstream>

typedef std::function<void (NvCodecVideoDecoderCuda::LogType, const std::string&)> LogFunc;

#ifdef __cuda_cuda_h__
inline bool check(const LogFunc& f, CUresult e, int iLine, const char* szFile) {
  if (e != CUDA_SUCCESS) {
    const char* szErrName = NULL;
    cuGetErrorName(e, &szErrName);
    std::stringstream ss;
    ss << "CUDA driver API error " << szErrName << " at line " << iLine
              << " in file " << szFile;
    f(NvCodecVideoDecoderCuda::LogType::LOG_ERROR, ss.str());
    return false;
  }
  return true;
}
#endif

#ifdef __CUDA_RUNTIME_H__
inline bool check(const LogFunc& f, cudaError_t e, int iLine, const char* szFile) {
  if (e != cudaSuccess) {
    std::stringstream ss;
    ss << "CUDA runtime API error " << cudaGetErrorName(e) << " at line "
              << iLine << " in file " << szFile;
    f(NvCodecVideoDecoderCuda::LogType::LOG_ERROR, ss.str());
    return false;
  }
  return true;
}
#endif

#ifdef _NV_ENCODEAPI_H_
inline bool check(const LogFunc& f, NVENCSTATUS e, int iLine, const char* szFile) {
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
    std::stringstream ss;
    ss << "NVENC error " << aszErrName[e] << " at line " << iLine
              << " in file " << szFile;
    f(NvCodecVideoDecoderCuda::LogType::LOG_ERROR, ss.str());
    return false;
  }
  return true;
}
#endif

#ifdef _WINERROR_
inline bool check(const LogFunc& f, HRESULT e, int iLine, const char* szFile) {
  if (e != S_OK) {
    std::stringstream ss;
    ss << "HRESULT error 0x" << (void*)e << " at line " << iLine
              << " in file " << szFile;
    f(NvCodecVideoDecoderCuda::LogType::LOG_ERROR, ss.str());
    return false;
  }
  return true;
}
#endif

#if defined(__gl_h_) || defined(__GL_H__)
inline bool check(const LogFunc& f, GLenum e, int iLine, const char* szFile) {
  if (e != 0) {
    std::stringstream ss;
    ss << "GLenum error " << e << " at line " << iLine << " in file "
              << szFile;
    f(NvCodecVideoDecoderCuda::LogType::LOG_ERROR, ss.str());
    return false;
  }
  return true;
}
#endif

inline bool check(const LogFunc& f, int e, int iLine, const char* szFile) {
  if (e < 0) {
    std::stringstream ss;
    ss << "General error " << e << " at line " << iLine << " in file "
              << szFile;
    f(NvCodecVideoDecoderCuda::LogType::LOG_ERROR, ss.str());
    return false;
  }
  return true;
}

#define ck(f, call) check(f, call, __LINE__, __FILE__)

std::unique_ptr<NvCodecVideoDecoderCuda> NvCodecVideoDecoderCuda::Create(cudaVideoCodec codec_id, std::function<void (LogType, const std::string&)> f) {
  auto p = std::unique_ptr<NvCodecVideoDecoderCuda>(new NvCodecVideoDecoderCuda());
  if (p->Init(codec_id, f) != 0) {
    return nullptr;
  }
  return p;
}

NvCodecVideoDecoderCuda::~NvCodecVideoDecoderCuda() {
  Release();
}

int32_t NvCodecVideoDecoderCuda::Init(cudaVideoCodec codec_id, const std::function<void (LogType, const std::string&)>& f) {
  if (!ck(f, cuInit(0))) {
    return -1;
  }
  int gpu_num = 0;
  if (!ck(f, cuDeviceGetCount(&gpu_num))) {
    return -2;
  }
  if (gpu_num == 0) {
    return -3;
  }
  if (!ck(f, cuDeviceGet(&cu_device_, 0))) {
    return -4;
  }
  if (!ck(f, cuCtxCreate(&cu_context_, 0, cu_device_))) {
    return -5;
  }

  try {
    decoder_.reset(new NvDecoder(cu_context_, false, codec_id, nullptr, false, false, nullptr, nullptr, 3840, 2160));
  } catch (NVDECException& e) {
    f(NvCodecVideoDecoderCuda::LogType::LOG_ERROR, e.what());
    return -4;
  }

  return 0;
}

void NvCodecVideoDecoderCuda::Decode(const uint8_t* ptr, int size, uint8_t**& frames, int& frame_count) {
  decoder_->Decode((const uint8_t*)ptr, size, &frames, &frame_count);
}

void NvCodecVideoDecoderCuda::Release() {
  decoder_.reset();
  cuCtxDestroy(cu_context_);
}