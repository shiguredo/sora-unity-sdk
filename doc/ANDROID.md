# Android で Sora Unity SDK を使ってみる

## Android で使うために必要な環境

- arm64-v8a のライブラリしか入れていないので armeabi-v7a の端末では動きません。
- 最低でも API LEVEL 24 （Android 7.0）が必要です。
- OpenGLES モードでは正常に動作しません。

## Android で使うために必要な設定

### Android Plugin の設定変更をします。

libSoraUnitySdk.so インスペクタの Platform settings -> Android の設定で CPU を ARM64 に変更して下さい。

[![Image from Gyazo](https://i.gyazo.com/f7dbf0ebbd1b1567517b4fcd34ff1c97.png)](https://gyazo.com/f7dbf0ebbd1b1567517b4fcd34ff1c97)

### Target Architectures で ARM64 を設定します。

Player Settings -> Other Settings -> Target Architectures で ARM64 にチェックをして下さい。

[![Image from Gyazo](https://i.gyazo.com/de434b5dfff683dd3f9c306b9e9844bc.png)](https://gyazo.com/de434b5dfff683dd3f9c306b9e9844bc)

### Vulkan で動かす必要があります。

Player Settings -> Other Settings の Graphics APIs で Vulkan を先頭にして下さい。

[![Image from Gyazo](https://i.gyazo.com/6088aa664df137c0bbeff69de2af57ae.png)](https://gyazo.com/6088aa664df137c0bbeff69de2af57ae)

## そのほかの利用に関する注意点

- Pixel 3 で解像度が 16 の倍数でない時に映像が乱れる問題があります。
  - [1084702 - Mobile Chrome on Pixel 3 has video corruption for non-16-aligned resolutions in WebRTC calls : Hardware VP8 encoder bug - chromium](https://bugs.chromium.org/p/chromium/issues/detail?id=1084702)
