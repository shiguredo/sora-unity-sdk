/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "device_video_capturer.h"

#include <math.h>
#include <stdint.h>
#include <memory>

#include "modules/video_capture/video_capture_factory.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"

namespace sora {

DeviceVideoCapturer::DeviceVideoCapturer() : vcm_(nullptr) {}

DeviceVideoCapturer::~DeviceVideoCapturer() {
  Destroy();
}

bool DeviceVideoCapturer::Init(size_t width,
                               size_t height,
                               size_t target_fps,
                               size_t capture_device_index) {
  std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> device_info(
      webrtc::VideoCaptureFactory::CreateDeviceInfo());

  char device_name[256];
  char unique_name[256];
  if (device_info->GetDeviceName(static_cast<uint32_t>(capture_device_index),
                                 device_name, sizeof(device_name), unique_name,
                                 sizeof(unique_name)) != 0) {
    Destroy();
    return false;
  }

  vcm_ = webrtc::VideoCaptureFactory::Create(unique_name);
  if (!vcm_) {
    return false;
  }
  vcm_->RegisterCaptureDataCallback(this);

#if defined(SORA_UNITY_SDK_HOLOLENS2)
  // HoloLens 2 の Capability は、ちょっとでも違う値だと無限ループに入ってしまうので、
  // 既存の Capability から一番近い値を拾ってくる
  int n = device_info->NumberOfCapabilities(vcm_->CurrentDeviceName());
  int diff = 0x7fffffff;
  for (int i = 0; i < n; i++) {
    webrtc::VideoCaptureCapability capability;
    device_info->GetCapability(vcm_->CurrentDeviceName(), i, capability);
    int d =
        abs((int)(width * height * target_fps -
                  capability.width * capability.height * capability.maxFPS));
    if (d < diff) {
      capability_ = capability;
      diff = d;
    }
  }
#else
  device_info->GetCapability(vcm_->CurrentDeviceName(), 0, capability_);
  capability_.width = static_cast<int32_t>(width);
  capability_.height = static_cast<int32_t>(height);
  capability_.maxFPS = static_cast<int32_t>(target_fps);
  capability_.videoType = webrtc::VideoType::kI420;
#endif

  if (vcm_->StartCapture(capability_) != 0) {
    Destroy();
    return false;
  }

  RTC_CHECK(vcm_->CaptureStarted());

  return true;
}

rtc::scoped_refptr<DeviceVideoCapturer> DeviceVideoCapturer::Create(
    size_t width,
    size_t height,
    size_t target_fps,
    std::string device_name) {
  rtc::scoped_refptr<DeviceVideoCapturer> capturer;
  std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
      webrtc::VideoCaptureFactory::CreateDeviceInfo());
  if (!info) {
    RTC_LOG(LS_WARNING) << "Failed to CreateDeviceInfo";
    return nullptr;
  }
  if (device_name.empty()) {
    // デバイス名の指定が無い場合、全部列挙して使えるのを利用する
    int num_devices = info->NumberOfDevices();
    for (int i = 0; i < num_devices; ++i) {
      capturer = Create(width, height, target_fps, i);
      if (capturer) {
        RTC_LOG(LS_WARNING) << "Get Capture";
        return capturer;
      }
    }
    RTC_LOG(LS_WARNING) << "Failed to create DeviceVideoCapturer";

    return nullptr;
  } else {
    // デバイス名の指定がある場合、デバイス名かユニーク名のどちらかにマッチしたものを利用する
    int num_devices = info->NumberOfDevices();
    for (int i = 0; i < num_devices; ++i) {
      char name[256];
      char unique_name[256];
      if (info->GetDeviceName(i, name, sizeof(name), unique_name,
                              sizeof(unique_name)) != 0) {
        RTC_LOG(LS_WARNING) << "Failed to GetDeviceName: index=" << i;
        continue;
      }
      if (device_name != name && device_name != unique_name) {
        continue;
      }

      capturer = Create(width, height, target_fps, i);
      if (!capturer) {
        RTC_LOG(LS_WARNING)
            << "Failed to Create video capturer:"
            << " specified_device_name=" << device_name
            << " device_name=" << name << " unique_name=" << unique_name
            << " width=" << width << " height=" << height
            << " target_fps=" << target_fps;
      }
      return capturer;
    }

    RTC_LOG(LS_WARNING) << "No specified video capturer found:"
                        << " specified_device_name=" << device_name
                        << " width=" << width << " height=" << height
                        << " target_fps=" << target_fps;
    return nullptr;
  }
}

rtc::scoped_refptr<DeviceVideoCapturer> DeviceVideoCapturer::Create(
    size_t width,
    size_t height,
    size_t target_fps,
    size_t capture_device_index) {
  rtc::scoped_refptr<DeviceVideoCapturer> vcm_capturer(
      new rtc::RefCountedObject<DeviceVideoCapturer>());
  if (!vcm_capturer->Init(width, height, target_fps, capture_device_index)) {
    RTC_LOG(LS_WARNING) << "Failed to create DeviceVideoCapturer(w = " << width
                        << ", h = " << height << ", fps = " << target_fps
                        << ")";
    return nullptr;
  }
  return vcm_capturer;
}

void DeviceVideoCapturer::Destroy() {
  if (!vcm_)
    return;

  vcm_->StopCapture();
  vcm_->DeRegisterCaptureDataCallback();
  // Release reference to VCM.
  vcm_ = nullptr;
}

void DeviceVideoCapturer::OnFrame(const webrtc::VideoFrame& frame) {
  OnCapturedFrame(frame);
}
}  // namespace sora