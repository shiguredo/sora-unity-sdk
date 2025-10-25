// D3D12 implementation of UnityCameraCapturer similar to D3D11 version
#include "unity_camera_capturer.h"

#ifdef SORA_UNITY_SDK_WINDOWS

#include <d3d12.h>
#include <dxgi1_4.h>
#include <windows.h>

#include "unity/IUnityGraphicsD3D12.h"

namespace sora_unity_sdk {

UnityCameraCapturer::D3D12Impl::~D3D12Impl() {
  if (fence_) {
    fence_->Release();
    fence_ = nullptr;
  }
  if (fence_event_) {
    CloseHandle(fence_event_);
    fence_event_ = nullptr;
  }
  if (cmd_list_) {
    cmd_list_->Release();
    cmd_list_ = nullptr;
  }
  if (cmd_allocator_) {
    cmd_allocator_->Release();
    cmd_allocator_ = nullptr;
  }
  if (readback_buffer_) {
    readback_buffer_->Release();
    readback_buffer_ = nullptr;
  }
}

bool UnityCameraCapturer::D3D12Impl::Init(UnityContext* context,
                                          void* camera_texture,
                                          int width,
                                          int height) {
  context_ = context;
  camera_texture_ = static_cast<ID3D12Resource*>(camera_texture);
  width_ = width;
  height_ = height;

  ID3D12Device* device = context->GetD3D12Device();
  ID3D12CommandQueue* queue = context->GetD3D12CommandQueue();
  if (device == nullptr || queue == nullptr) {
    RTC_LOG(LS_ERROR) << "IUnityGraphicsD3D12 interface not available";
    return false;
  }
  queue_ = queue;

  if (camera_texture_ == nullptr) {
    RTC_LOG(LS_ERROR) << "ID3D12Resource (camera_texture) is null";
    return false;
  }

  auto src_desc = camera_texture_->GetDesc();
  // BGRA8888 必須
  if (src_desc.Format != DXGI_FORMAT_B8G8R8A8_UNORM) {
    RTC_LOG(LS_WARNING)
        << "Unexpected D3D12 texture format; expected BGRA8. format="
        << static_cast<int>(src_desc.Format);
  }

  // camera_texture と同じサイズの readback 可能なテクスチャを作成
  UINT64 total_bytes = 0;
  D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
  UINT num_rows = 0;
  UINT64 row_size_in_bytes = 0;
  device->GetCopyableFootprints(&src_desc, 0, 1, 0, &footprint, &num_rows,
                                &row_size_in_bytes, &total_bytes);

  D3D12_HEAP_PROPERTIES heap_props = {};
  heap_props.Type = D3D12_HEAP_TYPE_READBACK;

  D3D12_RESOURCE_DESC buf_desc = {};
  buf_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  buf_desc.Alignment = 0;
  buf_desc.Width = total_bytes;
  buf_desc.Height = 1;
  buf_desc.DepthOrArraySize = 1;
  buf_desc.MipLevels = 1;
  buf_desc.Format = DXGI_FORMAT_UNKNOWN;
  buf_desc.SampleDesc.Count = 1;
  buf_desc.SampleDesc.Quality = 0;
  buf_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  buf_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

  HRESULT hr = device->CreateCommittedResource(
      &heap_props, D3D12_HEAP_FLAG_NONE, &buf_desc,
      D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&readback_buffer_));
  if (!SUCCEEDED(hr)) {
    RTC_LOG(LS_ERROR) << "CreateCommittedResource(READBACK) failed hr=" << hr;
    return false;
  }

  // コマンドアロケータとコマンドリストを作成
  hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                      IID_PPV_ARGS(&cmd_allocator_));
  if (!SUCCEEDED(hr)) {
    RTC_LOG(LS_ERROR) << "CreateCommandAllocator failed hr=" << hr;
    return false;
  }

  hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                 cmd_allocator_, nullptr,
                                 IID_PPV_ARGS(&cmd_list_));
  if (!SUCCEEDED(hr)) {
    RTC_LOG(LS_ERROR) << "CreateCommandList failed hr=" << hr;
    return false;
  }
  // フレーム毎に Reset するのでここで Close 状態にしておく
  cmd_list_->Close();

  // コマンドの実行を待つためのフェンスを作成
  hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
  if (!SUCCEEDED(hr)) {
    RTC_LOG(LS_ERROR) << "CreateFence failed hr=" << hr;
    return false;
  }

  fence_event_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  if (!fence_event_) {
    RTC_LOG(LS_ERROR) << "CreateEvent failed";
    return false;
  }

  readback_buffer_row_pitch_ = footprint.Footprint.RowPitch;
  readback_buffer_total_bytes_ = total_bytes;

  return true;
}

webrtc::scoped_refptr<webrtc::I420Buffer>
UnityCameraCapturer::D3D12Impl::Capture() {
  // コマンドリストの準備
  cmd_allocator_->Reset();
  cmd_list_->Reset(cmd_allocator_, nullptr);

  // camera_texture から readback_buffer_ にテクスチャをコピー
  D3D12_TEXTURE_COPY_LOCATION dst = {};
  dst.pResource = readback_buffer_;
  dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
  dst.PlacedFootprint.Footprint.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  dst.PlacedFootprint.Footprint.Width = width_;
  dst.PlacedFootprint.Footprint.Height = height_;
  dst.PlacedFootprint.Footprint.Depth = 1;
  dst.PlacedFootprint.Footprint.RowPitch = readback_buffer_row_pitch_;

  D3D12_TEXTURE_COPY_LOCATION src = {};
  src.pResource = camera_texture_;
  src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
  src.SubresourceIndex = 0;

  cmd_list_->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
  cmd_list_->Close();

  ID3D12CommandList* lists[] = {cmd_list_};
  queue_->ExecuteCommandLists(1, lists);

  // 実行したコマンドが反映されるまで待つ
  fence_value_++;
  queue_->Signal(fence_, fence_value_);
  if (fence_->GetCompletedValue() < fence_value_) {
    fence_->SetEventOnCompletion(fence_value_, fence_event_);
    WaitForSingleObject(fence_event_, INFINITE);
  }

  // readback_buffer_ からピクセルデータを読み込む
  const D3D12_RANGE read_range = {0, readback_buffer_total_bytes_};
  void* data = nullptr;
  HRESULT hr = readback_buffer_->Map(0, &read_range, &data);
  if (!SUCCEEDED(hr) || data == nullptr) {
    RTC_LOG(LS_ERROR) << "ID3D12Resource::Map failed hr=" << hr;
    return nullptr;
  }

  // Windows の場合は座標系の関係で上下反転してるので、頑張って元の向きに戻す
  std::unique_ptr<uint8_t[]> buf(new uint8_t[width_ * height_ * 4]);
  const uint8_t* base = static_cast<const uint8_t*>(data);
  const size_t pitch = readback_buffer_row_pitch_;
  for (int y = 0; y < height_; ++y) {
    const uint8_t* row = base + pitch * (height_ - 1 - y);
    std::memcpy(buf.get() + width_ * 4 * y, row, width_ * 4);
  }

  // ARGB -> I420 変換
  webrtc::scoped_refptr<webrtc::I420Buffer> i420_buffer =
      webrtc::I420Buffer::Create(width_, height_);
  libyuv::ARGBToI420(buf.get(), width_ * 4, i420_buffer->MutableDataY(),
                     i420_buffer->StrideY(), i420_buffer->MutableDataU(),
                     i420_buffer->StrideU(), i420_buffer->MutableDataV(),
                     i420_buffer->StrideV(), width_, height_);

  // writeback するデータは無いので空範囲を指定する
  const D3D12_RANGE written_range = {0, 0};
  readback_buffer_->Unmap(0, &written_range);

  return i420_buffer;
}

}  // namespace sora_unity_sdk

#endif  // SORA_UNITY_SDK_WINDOWS
