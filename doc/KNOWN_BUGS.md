# 既知のバグ

Sora Unity SDK では現在以下の既知のバグが存在します。

- 同一 PC で macOS 版 Sora Unity SDK サンプルと Safari でカメラを利用して送受信している場合、 Safari を切断すると Sora Unity SDK サンプルがクラッシュする
- UnityAudioOutput が ON の時に他の接続者が切断接続をすると音が出なくなる