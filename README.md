# Sora Unity SDK

**現在開発中で、ソースコードの公開は 2019 年 10 月末を予定しています**

Sora Unity SDK は [WebRTC SFU Sora](https://sora.shiguredo.jp/) の Unity クライアントアプリケーションを開発するためのライブラリです。

## About Support

We check PRs or Issues only when written in JAPANESE.
In other languages, we won't be able to deal with them. Thank you for your understanding.

## 進捗

[std::めるぽんさんはTwitterを使っています: 「WebRTC SFU Sora の Unity SDK 開発してるんだけど、無事マルチストリームが動くようになった https://t\.co/fHVEUaEsa7」 / Twitter](https://twitter.com/melponn/status/1185869961811513346?s=20)

## サポートについて

Sora Unity SDK に関する質問・要望・バグなどの報告は Issues の利用をお願いします。
ただし、 Sora のライセンス契約の有無に関わらず、 Issue への応答時間と問題の解決を保証しませんのでご了承ください。

Sora Unity SDK に対する有償のサポートについては現在提供しておりません。

## 対応予定機能

- [x] カメラから映像を取得し Sora で送信
- [x] カメラから映像を取得し Unity アプリに出力
- [x] マイクから音声を取得し Sora で送信
- [x] マイクから音声を取得し Unity アプリに出力
- [x] Unity アプリで Sora からの音声を受信
- [x] Unity アプリで Sora からの映像を受信
- [x] Sora から受信した音声を Unity アプリに出力
- [x] Sora から受信した映像を Unity アプリに出力
- [x] マルチストリームへの対応
- [x] ソフトウェアエンコード/デコード VP8 / VP9 への対応
    - ソフトウェアエンコード/デコードの H.264 へは非対応
- [x] Opus への対応
- [ ] シグナリング通知への対応
- [ ] サイマルキャスト対応

## 対応予定プラットフォーム

- [x] Windows 10 x86_64
- [ ] Windows 10 ARM64
- [ ] macOS x86_64

## オープンソースでの公開を前提とした有償による機能追加

**これら機能は継続的なメンテナンスの対象外となり、メンテナンスは有償での対応となります**

### NVIDIA NVENC / NVDEC 対応

- [ ] H.264 のハードウェアエンコードへの対応
- [ ] H.264 のハードウェアデコーダへの対応
- [ ] VP8 のハードウェアデコードへの対応
- [ ] VP9 のハードウェアデコードへの対応

### AMD Video Coding Engine 対応

- [ ] H.264 のハードウェアエンコードへの対応

### INTEL Quick Sync Video 対応

- [ ] H.264 のハードウェアエンコードへの対応
- [ ] VP8 のハードウェアエンコードへの対応
- [ ] VP9 のハードウェアエンコードへの対応
- [ ] H.264 のハードウェアデコードへの対応
- [ ] VP8 のハードウェアデコードへの対応
- [ ] VP9 のハードウェアデコードへの対応

### iOS 対応

**TBD**

### Android 対応

**TBD**

## ライセンス

Apache License 2.0

```
Copyright 2018-2019, Shiguredo Inc, melpon and kdxu

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
