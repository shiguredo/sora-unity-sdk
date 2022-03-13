#include "unity_camera_capturer.h"

namespace sora {

bool UnityCameraCapturer::NullImpl::Init(UnityContext* context,
                                         void* camera_texture,
                                         int width,
                                         int height) {
  width_ = width;
  height_ = height;
  return true;
}

rtc::scoped_refptr<webrtc::I420Buffer>
UnityCameraCapturer::NullImpl::Capture() {
  return webrtc::I420Buffer::Create(width_, height_);
}

}  // namespace sora
