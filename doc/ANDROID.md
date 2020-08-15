## Android で Sora Unity SDK を利用する際の注意点

### Android 利用のために必要な環境

- arm64-v8a のライブラリしか入れていないので armeabi-v7a の端末では動きません。
- 最低でも API LEVEL 24 （Android 7.0）が必要です。

### Android 利用のための設定変更

#### Android Plugin の設定変更をする必要があります。

- libSoraUnitySdk.so のインスペクタ -> Platform settings -> Android の設定で CPU が ARM64 になっていることを確認して下さい。
  [![Image from Gyazo](https://i.gyazo.com/42042dcc7dd506c1b42f0e7092992afc.png)](https://gyazo.com/42042dcc7dd506c1b42f0e7092992afc)

#### Target Architectures で ARM64 を設定する必要があります。

- Player Settings -> Other Settings -> Target Architectures で ARM64 にチェックが入っていることを確認して下さい。
  [![Image from Gyazo](https://i.gyazo.com/0bf7c9c091c635aa37084900aaff6fa0.png)](https://gyazo.com/0bf7c9c091c635aa37084900aaff6fa0)

#### Vulkan で動かす必要があります。

- Player Settings -> Other Settings の Graphics APIs で Vulkan が先頭にあることを確認して下さい。
  [![Image from Gyazo](https://i.gyazo.com/c13c6c9f600eb37d9d3b55caae30f190.png)](https://gyazo.com/c13c6c9f600eb37d9d3b55caae30f190)

### そのほかの利用に関する注意点

- Pixel 3 で解像度が 16 の倍数でない時に映像が乱れる問題があります。
  - [1084702 - Mobile Chrome on Pixel 3 has video corruption for non-16-aligned resolutions in WebRTC calls : Hardware VP8 encoder bug - chromium](https://bugs.chromium.org/p/chromium/issues/detail?id=1084702)
- OpenGLES モードでの動作は未確認です。
