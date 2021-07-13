# 変更履歴

- CHANGE
    - 下位互換のない変更
- UPDATE
    - 下位互換がある変更
- ADD
    - 下位互換がある追加
- FIX
    - バグ修正

## develop

- [ADD] `clientId` に対応し、 `Sora.config` に `ClientId` を追加
    - @torikizi
- [ADD] `spotlight_focus_rid` と `spotlight_unfocus_rid` に対応し、 `Sora.config` に以下のフィールドを追加
    - `SpotlightFocusRid`
    - `SpotlightUnfocusRid`
    - @torikizi
- [CHANGE] `Sora.Config` のフィールド名および `Sora.VideoCodec`, `Sora.AudioCodec` の名前を変更しました。本バージョンを利用する際には以下のように名前を変更してください。
    - `VideoCodec` → `VideoCodecType`
    - `VideoBitrate` → `VideoBitRate`
    - `AudioCodec` → `AudioCodecType`
    - `AudioBitrate` → `AudioBitRate`
    - @melpon
- [UPDATE] libwebrtc のバージョンを M91 (4472@{#0}) に上げる
    - @voluntas
- [UPDATE] libwebrtc のバージョンを M92 (4515@{#9}) に上げる
    - @melpon
- [ADD] プッシュ通知に対応
    - @melpon
- [ADD] DataChannel を使ったシグナリングに対応し、`Sora.Config` に以下のフィールドを追加
    - `EnableDataChannelSignaling`
    - `DataChannelSignaling`
    - `DataChannelSignalingTimeout`
    - `EnableIgnoreDisconnectWebsocket`
    - `IgnoreDisconnectWebsocket`
    - `DisconnectWaitTimeout`
    - @melpon
- [ADD] DataChannel を使ったメッセージングに対応し、`Sora.Config` に `List<DataChannelMessaging>` 型のフィールドを追加
    - @melpon
- [ADD] WS の re-offer に対応
    - @melpon

## 2021.2

- [UPDATE] WebRTC のバージョンを M90 (4430@{#3}) に上げる
    - @melpon
- [UPDATE] Boost のバージョンを 1.76.0 に上げる
    - @voluntas
- [ADD] AV1 に対応
    - @melpon

## 2021.1.2

- [FIX] OpenSSLCertificate では無くなったので BoringSSLCertificate を利用するように修正
    - TURN-TLS でセグフォする問題を解決
    - @melpon

## 2021.1.1

- [FIX] 4K 状態での回転を考慮して縦の最大サイズも 3840 にする
    - @melpon @torikizi
- [FIX] H.264 でデコードした際に内部的な縦横サイズが変わらないのを修正
    - @melpon @torikizi

## 2021.1

- [UPDATE] サイマルキャスト機能に対応する
    - @melpon @torikizi
- [UPDATE] スポットライト機能に対応する
    - @melpon @torikizi
- [UPDATE] WebRTC のバージョンを M89 (4389@{#5}) に上げる
    - @melpon @torikizi @voluntas
- [UPDATE] Boost のバージョンを 1.75.0 に上げる
    - @voluntas
- [UPDATE] nlohmann/json を Boost.JSON に変更
    - @melpon

## 2020.10

- [UPDATE] WebRTC のバージョンを M87 (4280@{#10}) に上げる
    - @melpon @torikizi
- [UPDATE] Boost のバージョンを 1.74.0 に上げる

## 2020.9

- [UPDATE] WebRTC のバージョンを M86 (4240@{#1}) に上げる
    - @voluntas
- [UPDATE] json を 3.9.1 に上げる
    - @voluntas

## 2020.8

- [ADD] RenderTexture に depth buffer を設定できるようにする
    - @melpon

## 2020.7

- [ADD] iOS に対応する
    - @melpon

## 2020.6.1

- [UPDATE] WebRTC のバージョンを M84 (4147@{#11}) に上げる
    - @voluntas
- [ADD] GetStats を Unity から呼べるようにする
    - @melpon
- [FIX] Android でビルドが通らなくなっていたのを `-fuse-ld=gold` にすることで修正
    - @melpon

## 2020.6

**リリースミスのためスキップ**

## 2020.5

- [UPDATE] Android 対応を正式版としてリリース
    - @melpon
- [UPDATE] WebRTC のバージョンを M84 (4147@{#10}) に上げる
    - @melpon
- [UPDATE] json を 3.8.0 に上げる
    - @voluntas
- [FIX] EncodedImage::set_buffer が消えてしまったのを修正
    - @melpon

## 2020.4

- [ADD] 実験的機能として Android に対応する
    - @melpon

## 2020.3.1

- [FIX] Windows で CUDA が無い環境でも動くようにする
    - @melpon

## 2020.3

- [UPDATE] Boost のバージョンを 1.73.0 に上げる
    - @voluntas
- [UPDATE] WebRTC のバージョンを M83 (4103@{#12}) に上げる
    - @melpon
- [CHANGE] Sora.Role.Upstream, Sora.Role.Downstream を削除
    - @melpon
- [FIX] 接続確立中に Sora.Dispose するとエラーになることがあったのを修正
    - @melpon
- [FIX] 接続が確立する前に ping を受け取ると通信が切断されてしまっていたのを修正
    - @melpon
- [FIX] Windows 版の H.264 デコードでリサイズが発生した際にエラーになるのを修正
    - @melpon

## 2020.2

- [UPDATE] WebRTC のバージョンを M81 (4044@{#12}) に上げる
    - @voluntas
- [ADD] Windows 版の H.264 デコードに NVIDIA VIDEO CODEC SDK を利用する
    - 実験的機能
    - @melpon

## 2020.1

- [ADD] Windows 版では H.264 エンコードに NVIDIA VIDEO CODEC SDK を利用する（利用可能な場合のみ）
    - 実験的機能
    - @melpon
- [ADD] macOS 版では H.264 のエンコード/デコードに VideoToolbox を利用する
    - 実験的機能
    - @melpon
- [ADD] Sora へ WebRTC 統計情報を送るようにする
    - @melpon

## 1.0.4

- [UPDATE] webrtc-build を 80.3987.2.2 に上げる
    - @melpon
- [CHANGE] Sora への接続時に sendonly, recvonly, sendrecv を指定できるようにする
    - @melpon

## 1.0.3

- [UPDATE] Boost のバージョンを 1.72.0 に上げる
    - @melpon
- [UPDATE] WebRTC のバージョンを M80 (3987@{#2}) に上げる
    - @melpon

## 1.0.2

- [UPDATE] WebRTC のバージョンを M79@{#5} に上げる
    - @melpon
- [UPDATE] Windows 版の WebRTC ライブラリを shiguredo-webrtc-build からダウンロードする
    - @melpon
- [UPDATE] macOS 版の WebRTC ライブラリを shiguredo-webrtc-build からダウンロードする
    - @melpon

## 1.0.1

- [ADD] connect 時の文字列に SDK や WebRTC のバージョンを送信する
    - @melpon

## 1.0.0

- [ADD] Windows 10 x86_64 対応
    - @melpon
- [ADD] macOS 10.15 対応
    - @melpon
- [ADD] macOS x86_64 でのデバイス指定機能を追加
    - @melpon
- [ADD] Windows x86_64 でのデバイス指定機能を追加
    - @melpon
- [ADD] カメラから映像を取得し Sora で送信
    - @melpon
- [ADD] カメラから映像を取得し Unity アプリに出力
    - @melpon
- [ADD] マイクから音声を取得し Sora で送信
    - @melpon
- [ADD] マイクから音声を取得し Unity アプリに出力
    - @melpon
- [ADD] Unity アプリで Sora からの音声を受信
    - @melpon
- [ADD] Unity アプリで Sora からの映像を受信
    - @melpon
- [ADD] Sora から受信した音声を Unity アプリに出力
    - @melpon
- [ADD] Sora から受信した映像を Unity アプリに出力
    - @melpon
- [ADD] マルチストリームへの対応
    - @melpon
- [ADD] VP8 / VP9 への対応
    - @melpon
- [ADD] Opus への対応
    - @melpon
- [ADD] Unity カメラからの映像取得に対応
    - @melpon
- [ADD] Sora のシグナリング通知に対応
    - @melpon
- [ADD] Sora の metadata に対応
    - @melpon
- [ADD] マイクの代わりに Unity からのオーディを出力に対応
    - @melpon
- [ADD] シグナリング開始時のコーデック指定機能を追加
    - @melpon
- [ADD] シグナリング開始時のビットレート指定機能を追加
    - @melpon
- [ADD] Unity 側で受信したオーディオを再生できるようにする
    - @melpon
- [ADD] 受信したオーディオを AudioClip から再生できるようにする
    - @melpon
