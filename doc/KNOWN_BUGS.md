# 既知のバグ

Sora Unity SDK では現在以下の既知のバグが存在します。適宜調査、対応を行なっています。

- 各プラットフォームの Unity SDK 同士で 3 接続以上相互接続をした時にクラッシュすることがある
- macOS の Sora Unity SDK を使って Safari と送受信している際に Safari を切断すると MacOS の Unity がクラッシュする
- UnityAudioOutput が ON の時に他の接続者が切断接続をすると音が出なくなる
- Windows の Unity SDK で H.264 の片方向受信時、送信側が getUserMedia から getDisplayMedia に切り替えると Unity がクラッシュする
