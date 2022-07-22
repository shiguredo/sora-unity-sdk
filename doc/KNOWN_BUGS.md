# 既知のバグ

Sora Unity SDK では現在以下の既知のバグが存在します。適宜調査、対応を行なっています。

- 各プラットフォームの Unity SDK 同士で 3 接続以上相互接続をした時にクラッシュすることがある
- 同一 PC で macOS 版 Sora Unity SDK サンプルと Safari でカメラを利用して送受信している場合、 Safari を切断すると Sora Unity SDK サンプルがクラッシュする
- UnityAudioOutput が ON の時に他の接続者が切断接続をすると音が出なくなる
- Windows の Unity SDK で H.264 の片方向受信時、送信側が getUserMedia から getDisplayMedia に切り替えると Unity がクラッシュする
