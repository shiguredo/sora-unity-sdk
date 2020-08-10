## iOS 版の利用上の注意点

- arm64 と x86_64 用バイナリしか生成しないので、32bit の iPhone には対応していません。
- 最低でも Target minimum iOS Version は 10.0 が必要です。
- iOS Plugin を利用するために設定変更をする必要があります。
  - libwebrtc.a のインスペクタ -> Select Platform for plugin -> Any Platform のチェックを外し、Include Platforms で iOS だけが チェックされるように設定して下さい。
  - libSoraUnitySdk.a のインスペクタ -> Select Platform for plugin -> Any Platform のチェックを外し、Include Platforms で iOS だけが チェックされるように設定して下さい。
- カメラを許可する必要があります。
  - Player Settings -> Other Settings -> Camera Usage Description にカメラ利用のためのコメントを設定して下さい。
- マイクを使用する場合許可する必要があります。( recvonly や Unity Audio Input を利用する場合は不要)
  - Player Settings -> Other Settings -> Microphone Usage Description にマイク利用のためのコメントを設定して下さい。
