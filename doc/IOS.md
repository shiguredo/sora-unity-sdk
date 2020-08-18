# iOS で Sora Unity SDK を使ってみる

## iOS で使うために必要な環境

- arm64 と x86_64 用バイナリしか生成しないので、32bit の iPhone には対応していません。
- 最低でも Target minimum iOS Version は 10.0 が必要です。

## iOS で使うために必要な設定

### iOS Plugin の設定変更をします。

- libwebrtc.a のインスペクタ -> Select Platform for plugin -> Any Platform のチェックを外し、 Include Platforms で iOS だけが チェックされるように設定して下さい。 [![Image from Gyazo](https://i.gyazo.com/56c8d6e6f975ae5666f25e07d0eccde9.png)](https://gyazo.com/56c8d6e6f975ae5666f25e07d0eccde9)

- libSoraUnitySdk.a のインスペクタ -> Select Platform for plugin -> Any Platform のチェックを外し、 Include Platforms で iOS だけが チェックされるように設定して下さい。 [![Image from Gyazo](https://i.gyazo.com/f7dbf0ebbd1b1567517b4fcd34ff1c97.png)](https://gyazo.com/f7dbf0ebbd1b1567517b4fcd34ff1c97)

### カメラの権限の設定をする必要があります。

Player Settings -> Other Settings -> Camera Usage Description にカメラ利用のためのコメントを設定して下さい。

[![Image from Gyazo](https://i.gyazo.com/ea332824fbcf5377734c6d399d1c77e2.png)](https://gyazo.com/ea332824fbcf5377734c6d399d1c77e2)

### マイクを使用する場合は権限の設定をする必要があります。

Player Settings -> Other Settings -> Microphone Usage Description にマイク利用のためのコメントを設定して下さい。(マイクを利用しない recvonly や Unity Audio Input の場合は不要です)

[![Image from Gyazo](https://i.gyazo.com/aa73f00db149a853234939659eff999a.png)](https://gyazo.com/aa73f00db149a853234939659eff999a)
