# Android で Sora Unity SDK を使ってみる

## 動作環境

- Android 7 以上が必要です
- arm64-v8a の端末が必要です

## Android で使うために必要な設定

### Android Plugin の設定変更をします

libSoraUnitySdk.so インスペクタの Platform settings -> Android の設定で CPU を ARM64 に変更して下さい。

[![Image from Gyazo](https://i.gyazo.com/f7dbf0ebbd1b1567517b4fcd34ff1c97.png)](https://gyazo.com/f7dbf0ebbd1b1567517b4fcd34ff1c97)

### Graphics APIs を設定します

#### Vulkan を使用したい場合

Player Settings -> Other Settings の Graphics APIs で Vulkan を先頭にして下さい。

[![Image from Gyazo](https://i.gyazo.com/bdd46d716499e312f3361b756e90b53c.png)](https://gyazo.com/bdd46d716499e312f3361b756e90b53c)

#### OpenGLES を使用したい場合

Player Settings -> Other Settings の Graphics APIs で OpenGLES3 を先頭にして下さい。

[![Image from Gyazo](https://i.gyazo.com/a3fe926948f72079cb663075c7968288.png)](https://gyazo.com/a3fe926948f72079cb663075c7968288)

### Minimum API Level で Android 7.0 'Nougat' ( API level 24 ) 以上を設定します

Player Settings -> Other Settings -> Minimum API Level で Android 7.0 'Nougat' ( API level 24 ) を以上を選択してください。

[![Image from Gyazo](https://i.gyazo.com/f14a796b9c2a1661cbb4ad39734b5cc5.png)](https://gyazo.com/f14a796b9c2a1661cbb4ad39734b5cc5)

### Target Architectures で ARM64 を設定します

Player Settings -> Other Settings -> Target Architectures で ARM64 にチェックをして下さい。

[![Image from Gyazo](https://i.gyazo.com/de434b5dfff683dd3f9c306b9e9844bc.png)](https://gyazo.com/de434b5dfff683dd3f9c306b9e9844bc)

## そのほかの利用に関する注意点

- Pixel 3 で解像度が 16 の倍数でない時に映像が乱れる問題があります。
    - [1084702 - Mobile Chrome on Pixel 3 has video corruption for non-16-aligned resolutions in WebRTC calls : Hardware VP8 encoder bug - chromium](https://bugs.chromium.org/p/chromium/issues/detail?id=1084702)
