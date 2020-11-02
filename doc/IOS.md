# iOS で Sora Unity SDK を使ってみる

## 動作環境

- iOS 10 以上が必要です
- 64bit の iPhone が必要です

## iOS で使うために必要な設定

### iOS Plugin の設定変更をします。

- libwebrtc.a のインスペクタ -> Select Platform for plugin -> Any Platform のチェックを外し、 Include Platforms で iOS だけが チェックされるように設定して下さい。 [![Image from Gyazo](https://i.gyazo.com/56c8d6e6f975ae5666f25e07d0eccde9.png)](https://gyazo.com/56c8d6e6f975ae5666f25e07d0eccde9)

- libSoraUnitySdk.a のインスペクタ -> Select Platform for plugin -> Any Platform のチェックを外し、 Include Platforms で iOS だけが チェックされるように設定して下さい。 [![Image from Gyazo](https://i.gyazo.com/622ba8940acb005eb354124b2a1999bb.png)](https://gyazo.com/622ba8940acb005eb354124b2a1999bb)

### Target Minimum iOS Version で 10.0 以上を設定します。

Player Settings -> Other Settings -> Target Minimum iOS Version で 10.0 以上を設定して下さい。

[![Image from Gyazo](https://i.gyazo.com/68c11b75729d369aba529e07129a61a6.png)](https://gyazo.com/68c11b75729d369aba529e07129a61a6)

### カメラを使用する場合は権限の設定をする必要があります。

Player Settings -> Other Settings -> Camera Usage Description にカメラ利用のためのコメントを設定して下さい。(カメラを利用しない recvonly や Capture Unity Camera の場合は不要です)

[![Image from Gyazo](https://i.gyazo.com/ea332824fbcf5377734c6d399d1c77e2.png)](https://gyazo.com/ea332824fbcf5377734c6d399d1c77e2)

### マイクを使用する場合は権限の設定をする必要があります。

Player Settings -> Other Settings -> Microphone Usage Description にマイク利用のためのコメントを設定して下さい。(マイクを利用しない recvonly や Unity Audio Input の場合は不要です)

[![Image from Gyazo](https://i.gyazo.com/aa73f00db149a853234939659eff999a.png)](https://gyazo.com/aa73f00db149a853234939659eff999a)
