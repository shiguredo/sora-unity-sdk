## iOS で Sora Unity SDK を利用する際の注意点

### iOS 利用のために必要な環境

- arm64 と x86_64 用バイナリしか生成しないので、32bit の iPhone には対応していません。
- 最低でも Target minimum iOS Version は 10.0 が必要です。

### iOS 利用のための設定変更

#### iOS Plugin の設定変更をする必要があります。

- libwebrtc.a のインスペクタ -> Select Platform for plugin -> Any Platform のチェックを外し、 Include Platforms で iOS だけが チェックされるように設定して下さい。
  [![Image from Gyazo](https://i.gyazo.com/e34f2d8c153e6962a3608baece18ee6c.png)](https://gyazo.com/e34f2d8c153e6962a3608baece18ee6c)

- libSoraUnitySdk.a のインスペクタ -> Select Platform for plugin -> Any Platform のチェックを外し、 Include Platforms で iOS だけが チェックされるように設定して下さい。
  [![Image from Gyazo](https://i.gyazo.com/1a7aba91aeed5bae303708d3d5d90ff0.png)](https://gyazo.com/1a7aba91aeed5bae303708d3d5d90ff0)

#### カメラの権限の設定をする必要があります。

- Player Settings -> Other Settings -> Camera Usage Description にカメラ利用のためのコメントを設定して下さい。
  [![Image from Gyazo](https://i.gyazo.com/e3e14212339c0cfc395a1bde53ee3593.png)](https://gyazo.com/e3e14212339c0cfc395a1bde53ee3593)

#### マイクを使用する場合は権限の設定をする必要があります。

- Player Settings -> Other Settings -> Microphone Usage Description にマイク利用のためのコメントを設定して下さい。(マイクを利用しない recvonly や Unity Audio Input の場合は不要です)
  [![Image from Gyazo](https://i.gyazo.com/7ef3d9f05e0582740252101463ff7465.png)](https://gyazo.com/7ef3d9f05e0582740252101463ff7465)
