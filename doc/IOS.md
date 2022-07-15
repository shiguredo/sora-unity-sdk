# iOS で Sora Unity SDK を使ってみる

## 動作環境

- iOS 12 以上が必要です
- 64bit の iPhone が必要です

## iOS で使うために必要な設定

### iOS Plugin の設定

#### libwebrtc.a

- インスペクタ -> Select Platform for plugin -> Any Platform のチェックを外し、 Include Platforms で iOS だけが チェックされるように設定してください。

[![Image from Gyazo](https://i.gyazo.com/7628eee5c7976abbc20eed5e9835b130.png)](https://gyazo.com/7628eee5c7976abbc20eed5e9835b130)

#### libSoraUnitySdk.a

- インスペクタ -> Select Platform for plugin -> Any Platform のチェックを外し、 Include Platforms で iOS だけが チェックされるように設定してください。
- Platform settings の OpenGLES の項目にチェックが入っていない場合、チェックを入れてください。

[![Image from Gyazo](https://i.gyazo.com/fc03999de53503d484f640e419701793.jpg)](https://gyazo.com/fc03999de53503d484f640e419701793)

#### libboost_json.a

- インスペクタ -> Select Platform for plugin -> Any Platform のチェックを外し、 Include Platforms で iOS だけが チェックされるように設定してください。

[![Image from Gyazo](https://i.gyazo.com/db3ddc8c4624d83cd2a0ed4cc65c4fbd.png)](https://gyazo.com/db3ddc8c4624d83cd2a0ed4cc65c4fbd)

#### libsora.a

- 特に変更は必要ありません。想定している設定内容は以下の画像の通りです。

[![Image from Gyazo](https://i.gyazo.com/d4de6b900211ed2fb528e3d245abc216.png)](https://gyazo.com/d4de6b900211ed2fb528e3d245abc216)


### Player Settings の設定

#### Target Minimum iOS Version

Player Settings -> Other Settings -> Target Minimum iOS Version で 12.0 以上を設定してください。

[![Image from Gyazo](https://i.gyazo.com/dfa0a38c8e5c78347a3030fb9603bd4e.png)](https://gyazo.com/dfa0a38c8e5c78347a3030fb9603bd4e)

#### カメラ使用時の設定

カメラを使用する場合は以下の設定をする必要があります。

Player Settings -> Other Settings -> Camera Usage Description にカメラ利用のためのコメント(内容は任意)を設定してください。カメラを利用しない recvonly や Capture Unity Camera の場合は不要です。

[![Image from Gyazo](https://i.gyazo.com/ea332824fbcf5377734c6d399d1c77e2.png)](https://gyazo.com/ea332824fbcf5377734c6d399d1c77e2)

#### マイク使用時の設定

マイクを使用する場合は以下の設定をする必要があります。

Player Settings -> Other Settings -> Microphone Usage Description にマイク利用のためのコメント(内容は任意)を設定して下さい。マイクを利用しない recvonly や Unity Audio Input の場合は不要です。

[![Image from Gyazo](https://i.gyazo.com/aa73f00db149a853234939659eff999a.png)](https://gyazo.com/aa73f00db149a853234939659eff999a)
