# 質問と答え

### 解像度を変更したい

- 解像度を変更したい場合はこちらの[SET_VIDEO_SIZE.md](https://github.com/shiguredo/sora-unity-sdk/blob/develop/doc/SET_VIDEO_SIZE.md)をお読みください。

### カメラを無効にして音声だけを送信したい

- 現在カメラを無効にすることは出来ませんが、 `video = false;` を指定することで音声だけを送信することが可能です。カメラデバイスを無効にしたい場合は fork して実装してください。

### AV1 を設定して配信したい

- 現在 AV1 が動作するのは Windows / macOS ( intel 64-bit ) / iOS / iPadOS のみです。 Android は動作しません。

### iOS または macOS から H.264 の FHD で配信したい

- iOS または macOS から FHD で配信したい場合は Sora の H.264 のプロファイルレベル ID を 5.2 以上に設定してください。 設定方法は [Sora のドキュメント](https://sora-doc.shiguredo.jp/SORA_CONF#8a25f5)をお読みください。プロファイルレベル ID を変更しない場合は H.264 の HD 以下で配信するか、他のコーデックを使用して FHD 配信をしてください。

### Sora の TURN 機能を無効にして利用したい

- Sora Unity SDK は Sora を `turn = false` に設定して利用することはできません。