# README の対応 OS バージョンを実態に合わせる

- Created: 2026-08-19
- Branch: feature/update-readme-supported-os-versions
- Polished: 2026-09-10

## 目的

README の「対応プラットフォーム」の記載を、実際にリンクする sora-cpp-sdk ライブラリの前提と一致させる。現状の記載は実態より広いサポートを謳っており、iOS 13 / macOS 13 / Android 7〜9 ではライブラリが動作しない。

## 現状

`README.md` と `SoraUnitySdkExamples/README.md` の「対応プラットフォーム」は次のとおり記載している（両ファイルに同じ内容が存在する）。

- Windows 10 22H2 x86_64 以降
- macOS 13.4.1 M1 以降
- Android 7 以降
- iOS 13 以降
- Ubuntu 22.04 x86_64
- Ubuntu 24.04 x86_64

一方、実際にリンクする sora-cpp-sdk のライブラリは iOS 14 / macOS 14 / Android 10 前提でビルドされている。

- iOS: `DEPS` の `WEBRTC_BUILD_VERSION=m150.7871.3.1` が指す webrtc-build の `_source/ios/webrtc/src/tools_webrtc/ios/build_ios_libs.py` の `IOS_MINIMUM_DEPLOYMENT_TARGET` が `14.0`（device / simulator とも `14.0`）
- macOS: 同じ webrtc-build の `DEPS` の `MACOS_DEPLOYMENT_TARGET=14`
- Android: sora-unity-sdk の `DEPS` の `ANDROID_NATIVE_API_LEVEL=29`（Android 10）

さらに、sora-cpp-sdk の TLS システム CA 化（2026.2.0 で導入。現依存の `DEPS` の `SORA_CPP_SDK_VERSION` は 2026.2.1 で、2026.2.0 の内容を含む）も macOS 14 / iOS 14 / Android 10 を対象としており、この下限とも一致している。

なお、sora-unity-sdk の iOS ビルドは run.py の `CMAKE_OSX_DEPLOYMENT_TARGET` で 13.0 を指定しているが、リンクするライブラリが 14.0 前提のため実質的に対象外である。

つまり、README の記載は実態より広いサポートを謳っており、iOS 13 / macOS 13 / Android 7〜9 ではライブラリが動作しない。README の記載と実態が乖離している。

## 設計方針

`README.md` と `SoraUnitySdkExamples/README.md` の「対応プラットフォーム」のうち、iOS / macOS / Android の下限を実態（iOS 14 以降、macOS 14 arm64 以降、Android 10 以降）に合わせて修正する。

- macOS 13.4.1 M1 以降 → macOS 14 arm64 以降
- Android 7 以降 → Android 10 以降
- iOS 13 以降 → iOS 14 以降

`run.py` の `AVAILABLE_TARGETS` に `macos_x86_64` は含まれておらず、macOS は arm64 のみを配布する。そのため `macOS 14 以降` と表記すると Intel Mac（Sonoma は Intel も対象）まで含む過大な記載になり、本 issue の目的を満たさない。アーキテクチャ表記は sora-cpp-sdk 2026.2.1 の README（`macOS 14 arm64 以降`）にあわせる。

Windows / Ubuntu は現状の記載（Windows 10 22H2 x86_64 以降 / Ubuntu 22.04 x86_64 / Ubuntu 24.04 x86_64）が sora-cpp-sdk の TLS システム CA 化の対象（Windows 10 以降 / Ubuntu 22.04・24.04）に含まれるため、そのまま維持する。

## 完了条件

- `README.md` の「対応プラットフォーム」の macOS / Android / iOS の下限がそれぞれ `macOS 14 arm64 以降` / `Android 10 以降` / `iOS 14 以降` に修正されている
- `SoraUnitySdkExamples/README.md` の「対応プラットフォーム」の macOS / Android / iOS の下限が同じく修正されている
- Windows / Ubuntu の記載は変更されていない

## 解決方法

未着手 (PR 作成後に追記する)
