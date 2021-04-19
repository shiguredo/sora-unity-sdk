# 質問と答え

### 解像度を変更したい

- 解像度を変更したい場合はこちらの[SET_VIDEO_SIZE.md](https://github.com/shiguredo/sora-unity-sdk/blob/develop/doc/SET_VIDEO_SIZE.md)をお読みください

### カメラを無効にして音声だけを送信したい

- 現在カメラを無効にすることは出来ません。Unity カメラキャプチャでダミーの映像を流すか、fork して実装して下さい。

### Android で Graphics APIs に Vulkan 以外を設定したい

- 現在 Graphics APIs を Vulkan 以外に設定することは出来ません。 Vulkan 以外を設定した場合最悪アプリがクラッシュします。 fork して実装して下さい。

### AV1 を設定して配信したい

- 現在 AV1 が動作するのは Windows/MacOS(intel)/iOS/iPadOS のみです。 Android は動作しません。
