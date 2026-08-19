# README の対応 OS バージョンを実態に合わせる

- Created: 2026-08-19
- Completed: {YYYY-MM-DD}
- Branch: feature/update-readme-supported-os-versions
- Polished: {YYYY-MM-DD}

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

- iOS: webrtc-build の `_source/ios/webrtc/src/tools_webrtc/ios/build_ios_libs.py` の `IOS_MINIMUM_DEPLOYMENT_TARGET` が `14.0`
- macOS: webrtc-build の `DEPS` の `MACOS_DEPLOYMENT_TARGET=14`
- Android: sora-unity-sdk の `DEPS` の `ANDROID_NATIVE_API_LEVEL=29`（Android 10）

さらに sora-cpp-sdk 2026.2.0 の TLS システム CA 化も macOS 14 / iOS 14 / Android 10 を対象としており、この下限とも一致している。

なお、sora-unity-sdk の iOS ビルドは run.py の `CMAKE_OSX_DEPLOYMENT_TARGET` で 13.0 を指定しているが、リンクするライブラリが 14.0 前提のため実質的に対象外である。

つまり、README の記載は実態より広いサポートを謳っており、iOS 13 / macOS 13 / Android 7〜9 ではライブラリが動作しない。README の記載と実態が乖離している。

## 設計方針

`README.md` と `SoraUnitySdkExamples/README.md` の「対応プラットフォーム」のうち、iOS / macOS / Android の下限を実態（iOS 14 以降、macOS 14 以降、Android 10 以降）に合わせて修正する。

- macOS 13.4.1 M1 以降 → macOS 14 以降
- Android 7 以降 → Android 10 以降
- iOS 13 以降 → iOS 14 以降

Windows / Ubuntu は sora-unity-sdk のサポート範囲が sora-cpp-sdk の TLS 対象範囲に含まれるため現状の記載を維持する。

## 完了条件

- `README.md` の「対応プラットフォーム」の macOS / Android / iOS の下限がそれぞれ `macOS 14 以降` / `Android 10 以降` / `iOS 14 以降` に修正されている
- `SoraUnitySdkExamples/README.md` の「対応プラットフォーム」の macOS / Android / iOS の下限が同じく修正されている
- Windows / Ubuntu の記載は変更されていない

## 解決方法

未着手 (PR 作成後に追記する)
