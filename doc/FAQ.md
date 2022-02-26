# 質問と答え

### 解像度を変更したい

- 解像度を変更したい場合はこちらの[SET_VIDEO_SIZE.md](https://github.com/shiguredo/sora-unity-sdk/blob/develop/doc/SET_VIDEO_SIZE.md)をお読みください

### カメラを無効にして音声だけを送信したい

- 現在カメラを無効にすることは出来ませんが、 `video = false;` を指定することで音声だけを送信することが可能です。カメラデバイスを無効にしたい場合は fork して実装して下さい。

### Android で Graphics APIs に Vulkan 以外を設定したい

- 現在 Graphics APIs を Vulkan 以外に設定することは出来ません。 Vulkan 以外を設定した場合最悪アプリがクラッシュします。 fork して実装して下さい。

### AV1 を設定して配信したい

- 現在 AV1 が動作するのは Windows / MacOS ( intel 64-bit ) / iOS / iPadOS のみです。 Android は動作しません。

### iOS 端末から H.264 の FHD で配信したい

- iOS 端末から FHD で配信したい場合は Sora の H.264 のプロファイルレベル ID を 3.2 以上に設定してください。 設定方法はこちらの [Sora のドキュメント](https://sora-doc.shiguredo.jp/sora_conf#default-h264-profile-level-id)をお読みください。プロファイルレベル ID を変更しない場合は H.264 の HD 以下で配信するか、他のコーデックを使用して FHD 配信をしてください。

### Unity 2020 (LTS) 以降で macOS 向けにビルドしたい

- Build Settings の Architecture を Intel-64bit にしてください。

### M1 Mac　を使用したい

- 現在 M1 Mac は未対応です。

### Pixel6 で H.264 を配信したい

- 現在 Pixel6 で H.264 を配信することはできません。 他のコーデックを使用してください。