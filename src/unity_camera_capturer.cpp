#include "unity_camera_capturer.h"

namespace sora {

rtc::scoped_refptr<UnityCameraCapturer> UnityCameraCapturer::Create(
    UnityContext* context,
    void* unity_camera_texture,
    int width,
    int height) {
  rtc::scoped_refptr<UnityCameraCapturer> p(
      new rtc::RefCountedObject<UnityCameraCapturer>());
  if (!p->Init(context, unity_camera_texture, width, height)) {
    return nullptr;
  }
  return p;
}

void UnityCameraCapturer::OnRender() {
#ifdef _WIN32
  D3D11_MAPPED_SUBRESOURCE resource;

  auto dc = context_->GetDeviceContext();
  if (dc == nullptr) {
    RTC_LOG(LS_ERROR) << "ID3D11DeviceContext is null";
    return;
  }

  // ピクセルデータが取れない（と思う）ので、カメラテクスチャから自前のテクスチャにコピーする
  dc->CopyResource((ID3D11Texture2D*)frame_texture_,
                   (ID3D11Resource*)camera_texture_);

  HRESULT hr =
      dc->Map((ID3D11Resource*)frame_texture_, 0, D3D11_MAP_READ, 0, &resource);
  if (!SUCCEEDED(hr)) {
    RTC_LOG(LS_ERROR) << "ID3D11DeviceContext::Map is failed: hr=" << hr;
    return;
  }

  // Windows の場合は座標系の関係で上下反転してるので、頑張って元の向きに戻す
  // TODO(melpon): Graphics.Blit を使って効率よく反転させる
  // (ref: https://github.com/Unity-Technologies/com.unity.webrtc/blob/a526753e0c18d681d20f3eb9878b9c28442e2bfb/Runtime/Scripts/WebRTC.cs#L264-L268)
  std::unique_ptr<uint8_t> buf(new uint8_t[width_ * height_ * 4]);
  for (int i = 0; i < height_; i++) {
    std::memcpy(buf.get() + width_ * 4 * i,
                ((const uint8_t*)resource.pData) +
                    resource.RowPitch * (height_ - i - 1),
                width_ * 4);
  }

  // I420 に変換して VideoFrame 作って OnFrame 呼び出し
  //RTC_LOG(LS_INFO) << "GOT FRAME: pData=0x" << resource.pData
  //                 << " RowPitch=" << resource.RowPitch
  //                 << " DepthPitch=" << resource.DepthPitch;
  rtc::scoped_refptr<webrtc::I420Buffer> i420_buffer =
      webrtc::I420Buffer::Create(width_, height_);
  //libyuv::ARGBToI420((const uint8_t*)resource.pData, resource.RowPitch,
  //                   i420_buffer->MutableDataY(), i420_buffer->StrideY(),
  //                   i420_buffer->MutableDataU(), i420_buffer->StrideU(),
  //                   i420_buffer->MutableDataV(), i420_buffer->StrideV(),
  //                   width_, height_);
  libyuv::ARGBToI420(buf.get(), width_ * 4, i420_buffer->MutableDataY(),
                     i420_buffer->StrideY(), i420_buffer->MutableDataU(),
                     i420_buffer->StrideU(), i420_buffer->MutableDataV(),
                     i420_buffer->StrideV(), width_, height_);
  auto video_frame = webrtc::VideoFrame::Builder()
                         .set_video_frame_buffer(i420_buffer)
                         .set_rotation(webrtc::kVideoRotation_0)
                         .set_timestamp_us(clock_->TimeInMicroseconds())
                         .build();
  this->OnFrame(video_frame);

  dc->Unmap((ID3D11Resource*)frame_texture_, 0);
#endif
}

void UnityCameraCapturer::OnFrame(const webrtc::VideoFrame& frame) {
  OnCapturedFrame(frame);
}

bool UnityCameraCapturer::Init(UnityContext* context,
                               void* unity_camera_texture,
                               int width,
                               int height) {
  context_ = context;
  camera_texture_ = unity_camera_texture;
  width_ = width;
  height_ = height;

#ifdef _WIN32
  auto device = context->GetDevice();
  if (device == nullptr) {
    return false;
  }

  // ピクセルデータにアクセスする用のテクスチャを用意する
  ID3D11Texture2D* texture = nullptr;
  D3D11_TEXTURE2D_DESC desc = {0};
  desc.Width = width;
  desc.Height = height;
  desc.MipLevels = 1;
  desc.ArraySize = 1;
  desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  desc.SampleDesc.Count = 1;
  desc.Usage = D3D11_USAGE_STAGING;
  desc.BindFlags = 0;
  desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  HRESULT hr = device->CreateTexture2D(&desc, NULL, &texture);
  if (!SUCCEEDED(hr)) {
    RTC_LOG(LS_ERROR) << "ID3D11Device::CreateTexture2D is failed: hr=" << hr;
    return false;
  }

  frame_texture_ = texture;
#endif
  return true;
}

}  // namespace sora
