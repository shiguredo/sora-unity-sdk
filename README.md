# Sora Unity SDK

[![libwebrtc](https://img.shields.io/badge/libwebrtc-m87.4280-blue.svg)](https://chromium.googlesource.com/external/webrtc/+/branch-heads/4280)
[![GitHub tag](https://img.shields.io/github/tag/shiguredo/sora-unity-sdk.svg)](https://github.com/shiguredo/sora-unity-sdk)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![Actions Status](https://github.com/shiguredo/sora-unity-sdk/workflows/build/badge.svg)](https://github.com/shiguredo/sora-unity-sdk/actions)

Sora Unity SDK は [WebRTC SFU Sora](https://sora.shiguredo.jp/) の Unity クライアントアプリケーションを開発するためのライブラリです。

## About Support

We check PRs or Issues only when written in JAPANESE.
In other languages, we won't be able to deal with them. Thank you for your understanding.

## 時雨堂のオープンソースソフトウェアについて

利用前に https://github.com/shiguredo/oss をお読みください。

## 利用方法

Sora Unity SDK を使ってみたい人は [USE.md](doc/USE.md) をお読みください。

## ビルド方法

- Windows でのビルド方法は [BUILD_WINDOWS.md](doc/BUILD_WINDOWS.md) をお読みください
- macOS でのビルド方法は [BUILD_MACOS.md](doc/BUILD_MACOS.md) をお読みください

## サンプル

[shiguredo/sora\-unity\-sdk\-samples: WebRTC SFU Sora Unity SDK サンプル集](https://github.com/shiguredo/sora-unity-sdk-samples)

### サンプル動作例

- [「こんな感じに Unity のカメラ映像を WebRTC で配信できるようになりました https://t\.co/R98ZmZTFOK」 / Twitter](https://twitter.com/melponn/status/1193406538494275592?s=20)
- [「ちゃんとリアルタイムで配信してます（モーション作るのが面倒だったのでシーンエディタから動かしてる）。Unity 側が配信で、ブラウザ（右上）で受信してる。 https://t\.co/TIL7NYroZm」 / Twitter](https://twitter.com/melponn/status/1193411591183552512?s=20)
- [「Momo (on Jetson Nano) -> Sora-Labo -> Sora-Unity と Sora-Js 同時受信。ここまでがお手軽すぎてやばい。」 / Twitter](https://twitter.com/izmhrats/status/1203299775354851328?s=20)

## 対応 Unity バージョン

**最新版と最新 LTS 版のみをサポート対応バージョンとします**

- Unity 2019.4
- Unity 2020.2

## 対応プラットフォーム

- Windows 10 1809 x86_64 以降
- macOS 10.15 x86_64 以降
- Android 7 以降
- iOS 10 以降

## 対応機能

- Windows への対応
- macOS への対応
- Android への対応
- iOS 対応
- Unity のカメラ映像を取得し Sora で送信
- カメラから映像を取得し Sora に送信
- カメラから映像を取得し Unity アプリに出力
- マイクから音声を取得し Sora に送信
- マイクから音声を取得し Unity アプリに出力
- Unity アプリで Sora からの音声を受信
- Unity アプリで Sora からの映像を受信
- Unity アプリで Sora からの音声を再生
- ソフトウェアエンコード/デコード VP8 / VP9 への対応
- Opus への対応
- デバイス指定機能
- マイクの代わりに Unity からのオーディオ出力
- Unity カメラからの映像取得に対応
- Unity 側で受信したオーディオの再生に対応
- Sora から受信した音声を Unity アプリに出力
- Sora から受信した映像を Unity アプリに出力
- Sora マルチストリーム機能への対応
- Sora シグナリング通知への対応
- Sora メタデータへの対応
- Sora シグナリング開始時の音声コーデック/ビットレート指定に対応
- Sora シグナリング開始時の映像コーデック/ビットレート指定に対応
- シグナリング通知への対応
- Apple VideoToolbox
    - H.264 ハードウェアエンコードへの対応
    - H.264 ハードウェアデコードへの対応
- NVIDIA VIDEO CODEC SDK
    - Windows 版
        - H.264 のハードウェアエンコードへの対応
        - H.264 のハードウェアデコードへの対応

## H.264 の利用について

Sora Unity SDK ではソフトウェアでの H.264 エンコード/デコードの利用はできません。
これは H.264 のソフトウェアエンコーダ/デコーダを含んで配布した場合はライセンス費用が発生することから、無効にしているためです。

そのため Windows では NVIDIA VIDEO CODEC SDK 、macOS では VideoToolbox を利用し、H.264 のエンコード/デコードを実現しています。また Android でも H.264 ハードウェアエンコーダを利用します。

詳細は H.264 を [USE_H264.md](doc/USE_H264.md) をお読みください

## 今後

- サイマルキャスト対応

## 有償での優先実装

- Windows 版 NVIDIA VIDEO CODEC SDK による H.264 エンコーダ対応
    - [スロースネットワークス株式会社](http://www.sloth-networks.co.jp) 様
- WebRTC's Statistics 対応
    - 企業名非公開
- Windows 版 NVIDIA VIDEO CODEC SDK による H.264 デコーダ対応
    - [スロースネットワークス株式会社](http://www.sloth-networks.co.jp) 様
- Android 版対応
    - [株式会社KDDIテクノロジー](https://kddi-tech.com/) 様

## 有償での優先実装が可能な機能一覧

**詳細は Discord またはメールにてお問い合わせください**

- オープンソースでの公開が前提
- 可能であれば企業名の公開
    - 公開が難しい場合は `企業名非公開` と書かせていただきます

### プラットフォーム

- Ubuntu 18.04 への対応

### NVIDIA VIDEO CODEC SDK

- VP8 のハードウェアデコードへの対応
- VP9 のハードウェアデコードへの対応
- Ubuntu 18.04 への対応

### INTEL Media SDK 対応

- H.264 のハードウェアエンコードへの対応
- VP8 のハードウェアエンコードへの対応
- VP9 のハードウェアエンコードへの対応
- H.264 のハードウェアデコードへの対応
- VP8 のハードウェアデコードへの対応
- VP9 のハードウェアデコードへの対応

## ライセンス

Apache License 2.0

```
Copyright 2019-2020, Wandbox LLC (Original Author)
Copyright 2019-2020, Shiguredo Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
```
