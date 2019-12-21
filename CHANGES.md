# 変更履歴

- UPDATE
    - 下位互換がある変更
- ADD
    - 下位互換がある追加
- CHANGE
    - 下位互換のない変更
- FIX
    - バグ修正

## develop

- [UPDATE] WebRTC のバージョンを M79@{#5} に上げた
- [UPDATE] Windows 版の WebRTC ライブラリを shiguredo-webrtc-build からダウンロードするようにした
- [UPDATE] macOS 版の WebRTC ライブラリを shiguredo-webrtc-build からダウンロードするようにした

## 1.0.1

- [ADD] connect 時の文字列に SDK や WebRTC のバージョンを送信する

## 1.0.0

- [ADD] Windows 10 x86_64 対応
- [ADD] macOS 10.15 対応
- [ADD] macOS x86_64 でのデバイス指定機能を追加
- [ADD] Windows x86_64 でのデバイス指定機能を追加
- [ADD] カメラから映像を取得し Sora で送信
- [ADD] カメラから映像を取得し Unity アプリに出力
- [ADD] マイクから音声を取得し Sora で送信
- [ADD] マイクから音声を取得し Unity アプリに出力
- [ADD] Unity アプリで Sora からの音声を受信
- [ADD] Unity アプリで Sora からの映像を受信
- [ADD] Sora から受信した音声を Unity アプリに出力
- [ADD] Sora から受信した映像を Unity アプリに出力
- [ADD] マルチストリームへの対応
- [ADD] VP8 / VP9 への対応
- [ADD] Opus への対応
- [ADD] Unity カメラからの映像取得に対応
- [ADD] Sora のシグナリング通知に対応
- [ADD] Sora の metadata に対応
- [ADD] マイクの代わりに Unity からのオーディを出力に対応
- [ADD] シグナリング開始時のコーデック指定機能を追加
- [ADD] シグナリング開始時のビットレート指定機能を追加
- [ADD] Unity 側で受信したオーディオを再生できるようにする
- [ADD] 受信したオーディオを AudioClip から再生できるようにする
