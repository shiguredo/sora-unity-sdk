# Android で Sora Unity SDK を使ってみる

## 動作環境

- Android 7 以上が必要です
- arm64-v8a の端末が必要です

## Android で使うために必要な設定

### Android Plugin の設定変更をします。

libSoraUnitySdk.so インスペクタの Platform settings -> Android の設定で CPU を ARM64 に変更して下さい。

[![Image from Gyazo](https://i.gyazo.com/f7dbf0ebbd1b1567517b4fcd34ff1c97.png)](https://gyazo.com/f7dbf0ebbd1b1567517b4fcd34ff1c97)

### iOS Plugin の設定変更をします。

- libwebrtc.a のインスペクタ -> Select Platform for plugin -> Any Platform のチェックを外し、 Include Platforms で iOS だけが チェックされるように設定して下さい。 [![Image from Gyazo](https://i.gyazo.com/56c8d6e6f975ae5666f25e07d0eccde9.png)](https://gyazo.com/56c8d6e6f975ae5666f25e07d0eccde9)

- libSoraUnitySdk.a のインスペクタ -> Select Platform for plugin -> Any Platform のチェックを外し、 Include Platforms で iOS だけが チェックされるように設定して下さい。 [![Image from Gyazo](https://i.gyazo.com/622ba8940acb005eb354124b2a1999bb.png)](https://gyazo.com/622ba8940acb005eb354124b2a1999bb)

### Graphics APIs を Vulkan に設定します。

Player Settings -> Other Settings の Graphics APIs で Vulkan を先頭にして下さい。

[![Image from Gyazo](https://i.gyazo.com/6088aa664df137c0bbeff69de2af57ae.png)](https://gyazo.com/6088aa664df137c0bbeff69de2af57ae)

### Minimum API Level で Android 7.0 'Nougat' ( API level 24 ) 以上を設定します。

Player Settings -> Other Settings -> Minimum API Level で Android 7.0 'Nougat' ( API level 24 ) を以上を選択してください。

[![Image from Gyazo](https://i.gyazo.com/f14a796b9c2a1661cbb4ad39734b5cc5.png)](https://gyazo.com/f14a796b9c2a1661cbb4ad39734b5cc5)

### Target Architectures で ARM64 を設定します。
Player Settings -> Other Settings -> Target Architectures で ARM64 にチェックをして下さい。

[![Image from Gyazo](https://i.gyazo.com/de434b5dfff683dd3f9c306b9e9844bc.png)](https://gyazo.com/de434b5dfff683dd3f9c306b9e9844bc)

## そのほかの利用に関する注意点

- Pixel 3 で解像度が 16 の倍数でない時に映像が乱れる問題があります。
  - [1084702 - Mobile Chrome on Pixel 3 has video corruption for non-16-aligned resolutions in WebRTC calls : Hardware VP8 encoder bug - chromium](https://bugs.chromium.org/p/chromium/issues/detail?id=1084702)

- OpenGLES モードでは一部機能が動作しないため、Vulkan モードで利用するのを推奨します。
