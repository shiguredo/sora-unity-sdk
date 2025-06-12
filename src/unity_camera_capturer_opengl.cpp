#include "unity_camera_capturer.h"

#if defined(SORA_UNITY_SDK_UBUNTU)
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#elif defined(SORA_UNITY_SDK_ANDROID)
#include <GLES2/gl2.h>
#endif

namespace sora_unity_sdk {

UnityCameraCapturer::OpenglImpl::~OpenglImpl() {
  if (fbo_ != 0) {
    glDeleteFramebuffers(1, &fbo_);
  }
}

#define GL_ERRCHECK_(name, value)                                            \
  {                                                                          \
    auto error = glGetError();                                               \
    if (error != GL_NO_ERROR) {                                              \
      RTC_LOG(LS_ERROR) << "Failed to " << name << ": error=" << (int)error; \
      return value;                                                          \
    }                                                                        \
  }

#define GL_ERRCHECK(name) GL_ERRCHECK_(name, false)

bool UnityCameraCapturer::OpenglImpl::Init(UnityContext* context,
                                           void* camera_texture,
                                           int width,
                                           int height) {
  camera_texture_ = camera_texture;
  width_ = width;
  height_ = height;
  return true;
}

#undef GL_ERRCHECK
#define GL_ERRCHECK(name) GL_ERRCHECK_(name, nullptr)

webrtc::scoped_refptr<webrtc::I420Buffer>
UnityCameraCapturer::OpenglImpl::Capture() {
  // Init 関数とは別のスレッドから呼ばれることがあるので、
  // ここに初期化処理を入れる
  if (!initialized_) {
    initialized_ = true;

    glGenFramebuffers(1, &fbo_);
    GL_ERRCHECK("glGenFramebuffers");

    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    GL_ERRCHECK("glBindFramebuffer");

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           (GLuint)(intptr_t)camera_texture_, 0);
    GL_ERRCHECK("glFramebufferTexture2D");
  }

  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
  GL_ERRCHECK("glBindFramebuffer");

  std::unique_ptr<uint8_t[]> buf(new uint8_t[width_ * height_ * 4]());

  glReadPixels(0, 0, width_, height_, GL_RGBA, GL_UNSIGNED_BYTE, buf.get());
  GL_ERRCHECK("glReadPixels");

  // OpenGL の座標は上下反転してるので、頑張って元の向きに戻す
  std::unique_ptr<uint8_t[]> buf2(new uint8_t[width_ * height_ * 4]);
  for (int i = 0; i < height_; i++) {
    std::memcpy(buf2.get() + width_ * 4 * i,
                buf.get() + width_ * 4 * (height_ - i - 1), width_ * 4);
  }

  webrtc::scoped_refptr<webrtc::I420Buffer> i420_buffer =
      webrtc::I420Buffer::Create(width_, height_);
  libyuv::ABGRToI420(buf2.get(), width_ * 4, i420_buffer->MutableDataY(),
                     i420_buffer->StrideY(), i420_buffer->MutableDataU(),
                     i420_buffer->StrideU(), i420_buffer->MutableDataV(),
                     i420_buffer->StrideV(), width_, height_);

  return i420_buffer;
}

}  // namespace sora_unity_sdk
