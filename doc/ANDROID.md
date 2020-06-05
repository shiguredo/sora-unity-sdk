## Android 版の利用上の注意点

- arm64-v8a のライブラリしか入れていないので armeabi-v7a の端末では動きません。
  - libSoraUnitySdk.so のインスペクタ -> Platform settings の Android の CPU が ARM64 になっていることを確認して下さい。
  - Player Settings -> Other Settings -> Target Architectures で ARM64 にチェックが入っていることを確認して下さい。
- Vulkan で動かす必要があります。
  - Player Settings -> Other Settings の Graphics APIs で Vulkan が先頭にあるか確認して下さい。
  - OpenGLES モードでの動作は未確認です。
- 最低でも API LEVEL 24 （Android 7.0）が必要です。
- Pixel 3 で解像度が 16 の倍数じゃない時に映像が乱れる問題があります。
  - [1084702 - Mobile Chrome on Pixel 3 has video corruption for non-16-aligned resolutions in WebRTC calls : Hardware VP8 encoder bug - chromium](https://bugs.chromium.org/p/chromium/issues/detail?id=1084702)
