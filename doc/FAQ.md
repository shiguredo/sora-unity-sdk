# 質問と答え

### 解像度を変更したい

- 解像度を変更したい場合はこちらの[SET_VIDEO_SIZE.md](https://github.com/shiguredo/sora-unity-sdk/blob/develop/doc/SET_VIDEO_SIZE.md)をお読みください。

### カメラを無効にして音声だけを送信したい

- 現在カメラを無効にすることは出来ませんが、 `video = false;` を指定することで音声だけを送信することが可能です。カメラデバイスを無効にしたい場合は fork して実装してください。

### カメラ無しの端末を利用して送受信をしたい

- 現在カメラ無しの端末で配信をすることはできません。カメラデバイスを無効にしたい場合と同様 fork して実装してください。

### AV1 を設定して配信したい

- 現在 AV1 が動作するのは Windows / macOS ( intel 64-bit ) / iOS / iPadOS のみです。 Android は動作しません。

### HTTP Proxy を利用して接続したい

- Sora.Config にて HTTP Proxy の設定を行えます
    - ProxyUrl : Proxy の URL とポート番号を指定します (例 : http://proxy.example.com:8080)
    - ProxyUsername : Proxy の認証に利用するユーザーを指定します
    - ProxyPassword : Proxy の認証に利用するパスワードを指定します

### iOS または macOS から H.264 の FHD で配信したい

- iOS または macOS から FHD で配信したい場合は Sora の H.264 のプロファイルレベル ID を 5.2 以上に設定してください。 設定方法は [Sora のドキュメント](https://sora-doc.shiguredo.jp/SORA_CONF#8a25f5)をお読みください。プロファイルレベル ID を変更しない場合は H.264 の HD 以下で配信するか、他のコーデックを使用して FHD 配信をしてください。

### Sora の TURN 機能を無効にして利用したい

- Sora Unity SDK は Sora を `turn = false` に設定して利用することはできません。

### Windows で 受信した VP9 の映像が表示されない

- NVIDIA 搭載の Windows で 128x128 未満の VP9 の映像を受信できません。詳細は [Sora C++ SDK の FAQ](https://github.com/shiguredo/sora-cpp-sdk/blob/develop/doc/faq.md#nvidia-%E6%90%AD%E8%BC%89%E3%81%AE-windows-%E3%81%A7-128x128-%E6%9C%AA%E6%BA%80%E3%81%AE-vp9-%E3%81%AE%E6%98%A0%E5%83%8F%E3%82%92%E5%8F%97%E4%BF%A1%E3%81%A7%E3%81%8D%E3%81%BE%E3%81%9B%E3%82%93) をご確認ください。
