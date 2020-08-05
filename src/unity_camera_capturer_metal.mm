#include "unity_camera_capturer.h"

#import <MetalKit/MetalKit.h>

// .mm ファイルからじゃないと IUnityGraphicsMetal.h を読み込めないので、ここで import する
#import "unity/IUnityGraphicsMetal.h"

namespace sora {

static std::string MTLStorageModeToString(MTLStorageMode mode) {
  switch (mode) {
    case MTLStorageModeShared:
      return "MTLStorageModeShared";
#if defined(SORA_UNITY_SDK_MACOS)
    case MTLStorageModeManaged:
      return "MTLStorageModeManaged";
#endif
    case MTLStorageModePrivate:
      return "MTLStorageModePrivate";
    //case MTLStorageModeMemoryless:
    //  return "MTLStorageModeMemoryless";
    default:
      return "Unknown";
  }
}

bool UnityCameraCapturer::MetalImpl::Init(UnityContext* context,
                                          void* camera_texture,
                                          int width,
                                          int height) {
  context_ = context;
  camera_texture_ = camera_texture;
  width_ = width;
  height_ = height;

  // camera_texture_ は private storage なので、getBytes でピクセルを取り出すことができない
  // なのでもう一個 shared storage なテクスチャを作り、キャプチャする時はそのテクスチャに転送して利用する
  auto tex = (id<MTLTexture>)camera_texture_;
  // 多分 MTLStorageModePrivate になる
  RTC_LOG(LS_INFO) << "Passed Texture width=" << tex.width
                   << " height=" << tex.height << " MTLStorageMode="
                   << MTLStorageModeToString(tex.storageMode);

  auto graphics = context_->GetInterfaces()->Get<IUnityGraphicsMetal>();
  auto device = graphics->MetalDevice();

  auto descriptor =
      [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:tex.pixelFormat
                                                         width:width_
                                                        height:height_
                                                     mipmapped:NO];
  auto tex2 = [device newTextureWithDescriptor:descriptor];
  // 多分 MTLStorageModeManaged になる
  RTC_LOG(LS_INFO) << "Created Texture width=" << tex2.width
                   << " height=" << tex2.height << " MTLStorageMode="
                   << MTLStorageModeToString(tex2.storageMode);
  frame_texture_ = tex2;
  return true;
}

rtc::scoped_refptr<webrtc::I420Buffer>
UnityCameraCapturer::MetalImpl::Capture() {
  auto camera_tex = (id<MTLTexture>)camera_texture_;
  auto tex = (id<MTLTexture>)frame_texture_;
  auto graphics = context_->GetInterfaces()->Get<IUnityGraphicsMetal>();

  graphics->EndCurrentCommandEncoder();

  auto commandBuffer = graphics->CurrentCommandBuffer();

  id<MTLBlitCommandEncoder> blit = [commandBuffer blitCommandEncoder];
  if (blit == nil) {
    return nullptr;
  }
  // [blit copyFromTexture:camera_tex toTexture:tex];
  [blit copyFromTexture:camera_tex
            sourceSlice:0
            sourceLevel:0
           sourceOrigin:MTLOriginMake(0, 0, 0)
             sourceSize:MTLSizeMake(width_, height_, 1)
              toTexture:tex
       destinationSlice:0
       destinationLevel:0
      destinationOrigin:MTLOriginMake(0, 0, 0)];
#if defined(SORA_UNITY_SDK_MACOS)
  [blit synchronizeResource:tex];
#endif
  [blit endEncoding];
  blit = nil;

  std::unique_ptr<uint8_t[]> buf(new uint8_t[width_ * height_ * 4]);
  auto region = MTLRegionMake2D(0, 0, width_, height_);
  [tex getBytes:buf.get()
      bytesPerRow:width_ * 4
       fromRegion:region
      mipmapLevel:0];

  // Metal の場合は座標系の関係で上下反転してるので、頑張って元の向きに戻す
  std::unique_ptr<uint8_t[]> buf2(new uint8_t[width_ * height_ * 4]);
  for (int i = 0; i < height_; i++) {
    std::memcpy(buf2.get() + width_ * 4 * i,
                buf.get() + width_ * 4 * (height_ - i - 1), width_ * 4);
  }

  rtc::scoped_refptr<webrtc::I420Buffer> i420_buffer =
      webrtc::I420Buffer::Create(width_, height_);
  libyuv::ARGBToI420(buf2.get(), width_ * 4, i420_buffer->MutableDataY(),
                     i420_buffer->StrideY(), i420_buffer->MutableDataU(),
                     i420_buffer->StrideU(), i420_buffer->MutableDataV(),
                     i420_buffer->StrideV(), width_, height_);

  return i420_buffer;
}
}
