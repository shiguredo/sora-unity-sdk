# src/unity_camera_capturer.h と src/unity_renderer.h の IWYU 違反を解消する

- Priority: Low
- Created: 2026-08-27
- Branch: fmt/fix-header-iwyu
- Polished: 2026-09-11

## 目的

`src/unity_camera_capturer.h` と `src/unity_renderer.h` のヘッダ本文で使用していない include を削除し、対応する `.cpp` / `.mm` 側で必要な include を明示することで IWYU (Include What You Use) 違反を解消する。

## 現状

`src/unity_camera_capturer.h` には次の include が並んでいるが、ヘッダ本文からは参照されていない。

- `#include <libyuv.h>`
- `#include <rtc_base/logging.h>`

これらは各 `unity_camera_capturer_*.cpp` / `.mm` から透過的に利用されており、各ファイル側で明示的に include されていない。具体的には次のとおり。

- `unity_camera_capturer_d3d11.cpp` / `unity_camera_capturer_d3d12.cpp` / `unity_camera_capturer_metal.mm` / `unity_camera_capturer_opengl.cpp` / `unity_camera_capturer_vulkan.cpp` は `RTC_LOG` と `libyuv::ARGBToI420` / `libyuv::ABGRToI420`、および `webrtc::I420Buffer::Create` を使用している
- `unity_camera_capturer.cpp` は `RTC_LOG` のみ使用している（libyuv は使用しない）

`src/unity_renderer.h` も同様に、以下がヘッダ本文で未使用のまま残っている。

- `#include <api/video/i420_buffer.h>`（ヘッダ本文では `webrtc::I420Buffer` を参照していない）
- `#include <libyuv.h>`

`src/unity_renderer.cpp` の `Sink::TextureUpdateCallback` は `webrtc::I420Buffer::Create` と `libyuv::I420ToABGR` を使用しているが、`api/video/i420_buffer.h` と `libyuv.h` を直接 include していない（`rtc_base/logging.h` は直接 include 済み）。

ヘッダの依存が実態と乖離することでコンパイル時間が伸び、依存関係が読みにくくなる。

## 設計方針

- `src/unity_camera_capturer.h` から `libyuv.h` と `rtc_base/logging.h` を削除する
- `src/unity_renderer.h` から `api/video/i420_buffer.h` と `libyuv.h` を削除する
- 各 `unity_camera_capturer_*.cpp` / `.mm` に `rtc_base/logging.h` / `libyuv.h` / `api/video/i420_buffer.h` を、`unity_camera_capturer.cpp` に `rtc_base/logging.h` を、`src/unity_renderer.cpp` に `api/video/i420_buffer.h` / `libyuv.h` を、使用するものだけ明示的に追加する
- ビルドが通ることを全ターゲットで確認する
- 挙動変更は無い

## 完了条件

- `src/unity_camera_capturer.h` から `#include <libyuv.h>` と `#include <rtc_base/logging.h>` が消えている
- `src/unity_renderer.h` から `#include <api/video/i420_buffer.h>` と `#include <libyuv.h>` が消えている
- 使用している側（`unity_camera_capturer_*.cpp` / `.mm` と `unity_renderer.cpp`）で、使用している `RTC_LOG` / `libyuv::ARGBToI420` / `libyuv::ABGRToI420` / `libyuv::I420ToABGR` / `webrtc::I420Buffer` を提供する include が明示されている
- Windows / macOS / iOS / Android / Ubuntu の全ターゲットでビルドが通る
