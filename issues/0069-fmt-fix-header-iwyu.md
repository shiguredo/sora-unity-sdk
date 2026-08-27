# src/unity_camera_capturer.h と src/unity_renderer.h の IWYU 違反を解消する

- Priority: Low
- Created: 2026-08-27
- Branch: fmt/fix-header-iwyu
- Polished: {YYYY-MM-DD}

## 目的

`src/unity_camera_capturer.h` と `src/unity_renderer.h` のヘッダ本文で使用していない include を削除し、対応する `.cpp` / `.mm` 側で必要な include を明示することで IWYU (Include What You Use) 違反を解消する。

## 現状

`src/unity_camera_capturer.h` には次の include が並んでいるが、ヘッダ本文からは参照されていない。

- `#include <libyuv.h>`
- `#include <rtc_base/logging.h>`

これらは各 `unity_camera_capturer_*.cpp` / `.mm` から透過的に利用されており、cpp 側で明示的に include されていない。

`src/unity_renderer.h` も同様に、以下がヘッダ本文で未使用のまま残っている。

- `#include <api/video/i420_buffer.h>`（ヘッダ本文は `webrtc::VideoFrameBuffer` にしか触れない）
- `#include <libyuv.h>`

ヘッダの依存が実態と乖離することでコンパイル時間が伸び、依存関係が読みにくくなる。

## 設計方針

- `src/unity_camera_capturer.h` の未使用 include 2 件を削除する
- `src/unity_renderer.h` の未使用 include 2 件を削除する
- 対応する `.cpp` / `.mm` 側で必要な include を明示的に追加する
- ビルドが通ることを全ターゲットで確認する
- 挙動変更は無い

## 完了条件

- 対象 2 ヘッダの include 宣言に未使用のものが残っていない
- 対応する `.cpp` / `.mm` 側で `libyuv.h` / `rtc_base/logging.h` / `api/video/i420_buffer.h` などが必要に応じて明示 include されている
- Windows / macOS / iOS / Android / Ubuntu の全ターゲットでビルドが通る
