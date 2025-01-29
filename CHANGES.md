# 変更履歴

- CHANGE
  - 後方互換性のない変更
- UPDATE
  - 後方互換性がある変更
- ADD
  - 後方互換性がある追加
- FIX
  - バグ修正

## develop

- [UPDATE] Sora C++ SDK を `2025.2.0` に上げる
  - `CMAKE_VERSION` を `3.31.4` にアップデート

## 2025.1.0

**リリース日**: 2025-01-29

- [CHANGE] `boost::optional` を利用していた部分を `std::optional` に変更
  - Sora C++ SDK 2024.8.0 での変更に追従
  - @torikizi
- [CHANGE] CI の Ubuntu から libdrm-dev と libva-dev をインストールしないようにする
  - @torikizi
- [CHANGE] CMakefile の依存から libva と libdrm を削除する
  - @torikizi
- [CHANGE] ForwardingFilter は非推奨であるため Obsolete を追加
  - 今後は ForwardingFilters を利用するように促すメッセージを追加
  - @torikizi
- [UPDATE] Sora C++ SDK を `2025.1.0` に上げる
  - WEBRTC_BUILD_VERSION を `m132.6834.5.2` にアップデート
  - BOOST_VERSION を 1.87.0 にアップデート
    - boost::system::error_code が削除されたので boost::system::error_code に置き換える
  - `rtc::CreateRandomString` のヘッダを追加
  - `SetRtpTimestamp` を `frame.timestamp` から `frame.rtp_timestamp` に変更
  - `CMAKE_VERSION` を `3.30.5` にアップデート
  - @torikizi
- [ADD] `Sora.Config` に `ClientCert`, `ClientKey`, `CACert` を追加
  - @melpon
- [ADD] DataChannels に `Header` を追加
  - @torikizi
- [ADD] ForwardingFilter に name と priority を追加
  - @torikizi
- [ADD] ForwardingFilters 機能を使えるようにする
  - @torikizi

### misc

- [CHANGE] GitHub Actions の `ubuntu-latest` を `ubuntu-24.04` に変更する
  - @voluntas
- [UPDATE] Github Actions の `macos-12` を `macos-14` に変更する
  - @miosakuma
- [CHANGE] canary リリースの仕組みを導入する
  - canary リリースの時は prerelease フラグをつけるように変更
  - @torikizi

## 2024.4.0 (2024-07-29)

- [CHANGE] `--sora-dir`, `--sora-args` を `--local-sora-cpp-sdk-dir` と `--local-sora-cpp-sdk-args` に変更する
  - @melpon
- [CHANGE] `--webrtc-build-dir`, `--webrtc-build-args` を `--local-webrtc-build-dir` と `--local-webrtc-build-args` に変更する
  - @melpon
- [UPDATE] Sora C++ SDK を `2024.7.0` に上げる
  - Sora C++ SDK に追従して H.264 専用となっていた `NvCodecH264Encoder` を `NvCodecVideoEncoder` に変更する
  - WEBRTC_BUILD_VERSION を `m127.6533.1.1` にアップデート
  - BOOST_VERSION を 1.85.0 にアップデート
  - CMAKE_VERSION を 3.29.6 にアップデート
  - @melpon @torikizi
- [UPDATE] Github Actions で利用する Windows を 2022 にあげる
  - @torikizi
- [ADD] Ubuntu 22.04 でのビルドに対応する
  - 単にローカルビルド可能にしただけで、リリースには含めていない
  - @melpon
- [ADD] WebSocket での接続時に User-Agent を設定する
  - @melpon

## 2024.3.0 (2024-04-18)

- [UPDATE] Sora C++ SDK を `2024.6.1` に上げる
  - Intel VPL H.265 対応（ Windows, Ubuntu 22.04 ）
  - WEBRTC_BUILD_VERSION を `m122.6261.1.0` にあげる
    - Ubuntu のビルドを通すために、 __assertion_handler というファイルをコピーする処理を追加した
  - @miosakuma @torikizi
- [UPDATE] run.py に定義されていた関数を buildbase.py に移動する
  - @melpon
- [UPDATE] Github Actions の actions/cache , actions/upload-artifact , actions/download-artifact をアップデート
  - Node.js 16 の Deprecated に伴うアップデート
    - actions/cache@v3 から actions/cache@v4 にアップデート
    - actions/upload-artifact@v3 から actions/upload-artifact@v4 にアップデート
    - actions/download-artifact@v3 から actions/download-artifact@v4 にアップデート
  - @torikizi
- [UPDATE] 対応プラットフォームに Ubuntu 22.04 x86_64 を追加する
  - コード変更はなし、Ubuntu 20.04 版の Unity SDK で Ubuntu 22.04 も動作することを確認した
  - @torikizi

## 2024.2.0 (2024-03-13)

- [CHANGE] Lyra を削除
  - Sora.Config から AudioCodecLyraBitrate を削除
  - Sora.Config から AudioCodecLyraUsedtx を削除
  - Sora.Config から CheckLyraVersion を削除
  - Sora.AudioCodecType から LYRA を削除
  - VERSION ファイルから LYRA_VERSION を削除
  - @melpon
- [CHANGE] AudioOutputHelper を IAudioOutputHelper に変更
  - Android のハンズフリー機能の追加に伴い、AudioOutputHelper を IAudioOutputHelper に変更
  - これは破壊的な変更になるため、既存の AudioOutputHelper を利用している場合は、IAudioOutputHelper に変更が必要になる
  - 以下のような理由から IAudioOutputHelper へ変更
  - iOS と Android の異なるハンズフリー機能を同一の実装で呼び出せるように、現状のハンズフリー機能の呼び出し方法を変更
    - Sora.AudioOutputHelper の生成を Factory にて行う
    - AudioOutputHelper(Action onChangeRoute) から AudioOutputHelperFactory.create(Action onChangeRoute) に変更
    - Sora.AudioOutputHelper は interface となるため Sora.IAudioOutputHelper に変更
  - @tnoho
- [UPDATE] Sora C++ SDK を `2024.4.0` に上げる
  - Sora C++ SDK 2024.3.1 アップデート時に関連するライブラリもアップデート
  - WEBRTC_BUILD_VERSION を `m121.6167.3.0` にアップデート
  - BOOST_VERSION を 1.84.0 にアップデート
  - CMAKE_VERSION を 3.28.1 にアップデート
  - @melpon @miosakuma @torikizi
- [ADD] Android 向けのハンズフリー機能を追加
  - `Sora.cs` に Android 向けのハンズフリー機能を追加
  - iOS と同様に Android でもハンズフリー機能の利用を可能にする
  - @tnoho
- [ADD] run.py に --sora-dir と --sora-args オプションを追加
  - @melpon
- [ADD] run.py に --webrtc-build-dir と --webrtc-build-args オプションを追加
  - @melpon

## 2024.1.0 (2024-01-22)

- [UPDATE] Sora C++ SDK を `2024.1.0` に上げる
  - WEBRTC_BUILD_VERSION を `m120.6099.1.2` にアップデート
  - @torikizi
- [ADD] VideoCodecType に H265 を追加
  - @torikizi
- [ADD] ForwardingFilter に version と metadata を追加
  - @torikizi
- [FIX] ForwardingFilter の action をオプション項目に変更
  - Sora C++ SDK の変更に追従
  - @torikizi

## 2023.5.2 (2023-12-02)

- [FIX] Sora C++ SDK を `2023.16.1` に上げる
  - 修正取り込みのため Sora C++ SDK 2023.16.1 にアップデート
  - Apple 非公開 API を利用していたため、Apple からリジェクトされる問題を修正
  - @voluntas

## 2023.5.1 (2023-11-30)

- [FIX] テクスチャのアップデート中に Sink が削除されてクラッシュすることがあるのを修正
  - @melpon

## 2023.5.0 (2023-11-13)

- [UPDATE] Sora C++ SDK を `2023.15.1` に上げる
  - 修正取り込みのため Sora C++ SDK 2023.15.1 にアップデート
  - Sora C++ SDK 2023.15.0 アップデート時に関連するライブラリもアップデート
  - WEBRTC_BUILD_VERSION を `m119.6045.2.1` にアップデート
  - CMAKE_VERSION を `3.27.7` にアップデート
  - ANDROID_NDK_VERSION を `r26b` にアップデート
  - @torikizi
- [UPDATE] パッケージディレクトリ変更に追従する
  - WEBRTC_BUILD_VERSION のアップデートに伴い、パッケージディレクトリが変更されたためそれに追従する
  - @torikizi

## 2023.4.0 (2023-10-26)

- [CHANGE] `Sora.Config` 中にあるキャプチャラに関するフィールドを `Sora.CameraConfig` に移動する
  - 修正方法は [Sora Unity SDK ドキュメント](https://sora-unity-sdk.shiguredo.jp/2023_3_0_to_2023_4_0) を参照して下さい
  - @melpon
- [UPDATE] SoraClientContext を利用してコードを短くする
  - @melpon
- [UPDATE] libwebrtc を `m117.5938.2.0` に上げる
  - @melpon @torikizi
- [UPDATE] Sora C++ SDK を `2023.14.0` に上げる
  - @melpon @torikizi
- [ADD] iOS デバイスのハンズフリーを可能とする `AudioOutputHelper` を追加
  - @melpon
- [ADD] 接続中にキャプチャラを切り替える機能を実装
  - @melpon
- [ADD] デバイスを掴まないようにする `NoVideoDevice`, `NoAudioDevice` を追加
  - @melpon
- [ADD] ハードウェアエンコーダを利用するかどうかを設定する `UseHardwareEncoder` を追加
  - @melpon
- [ADD] `SelectedSignalingURL` と `ConnectedSignalingURL` プロパティを追加
  - @melpon
- [FIX] IosAudioInit を初回接続の場合のみ呼び出すようにすることで、iOS で連続して接続しようとすると落ちることがあったのを修正
  - @melpon
- [FIX] AudioOutputHelper.Dispose() を複数回呼んでもクラッシュしないように修正
  - @melpon

## 2023.3.0 (2023-08-08)

- [UPDATE] Sora C++ SDK を `2023.10.0` に上げる
  - @torikizi @voluntas
- [UPDATE] libwebrtc を `m115.5790.7.0` に上げる
  - @torikizi

## 2023.2.0 (2023-07-19)

- [UPDATE] libwebrtc を `m114.5735.2.0` に上げる
  - @torikizi @miosakuma
- [UPDATE] Sora C++ SDK を `2023.7.2` に上げる
  - @torikizi @miosakuma
- [ADD] ForwardingFilter 機能を使えるようにする
  - @melpon
- [ADD] CodecParams 機能を使えるようにする
  - @torikizi
- [FIX] GetStats でデータレースによりエラーが発生するケースについて修正する
  - @melpon

## 2023.1.0 (2023-04-13)

- [UPDATE] Sora C++ SDK を `2023.4.0` に上げる
  - @melpon @miosakuma
- [UPDATE] libwebrtc を `m111.5563.4.4` に上げる
  - @melpon
- [UPDATE] Boost を `1.81.0` に上げる
  - @melpon
- [UPDATE] CMake を `3.25.1` に上げる
  - @melpon
- [UPDATE] actions/checkout@v2 と actions/cache@v2 を @v3 に上げる
  - @melpon
- [UPDATE] actions/upload-artifact@v2 と actions/download-artifact@v2 を @v3 に上げる
  - @melpon
- [ADD] `Sora.Config` に `AudioStreamingLanguageCode` を追加
  - @melpon
- [ADD] `Sora.Config` に `SignalingNotifyMetadata` を追加
  - @melpon
- [ADD] offer 設定時のコールバック関数 `OnSetOffer` を追加
  - @melpon
- [ADD] オーディオコーデックに Lyra を追加
  - @melpon
- [ADD] `Sora.Config` に `check_lyra_version` を追加
  - @torikizi

## 2022.6.2 (2023-03-25)

- [FIX] IPHONE_DEPLOYMENT_TARGET を 13 に上げる
  - @melpon

## 2022.6.1 (2023-03-24)

- [FIX] Sora C++ SDK を `2022.12.4` に上げる
  - Unity で発生した、Websocket 切断時にクラッシュする不具合を修正
  - Sora C++ SDK で WS 切断時のタイムアウトが起きた際に無効な関数オブジェクトが呼ばれていた
  - @miosakuma

## 2022.6.0 (2022-12-08)

- [ADD] 実行時に音声と映像のミュート、ミュート解除をする機能を追加
  - @melpon

## 2022.5.2 (2022-10-04)

- [FIX] UnityCameraCapturer がマルチスレッド下で正常に終了しないことがあるのを修正
  - @melpon

## 2022.5.1 (2022-09-24)

- [UPDATE] Sora C++ SDK を `2022.12.1` に上げる
  - @melpon
- [FIX] カメラからのフレーム情報が縮小された状態で渡されていたのを修正
  - @melpon

## 2022.5.0 (2022-09-22)

- [ADD] カメラからのフレーム情報を受け取るコールバックを追加
  - @melpon
- [ADD] カメラデバイスの FPS の設定可能にする
  - @melpon

## 2022.4.0

- [CHANGE] iOS ビルド向けに Bitcode を off にする設定を追加
  - @torikizi
- [UPDATE] Sora C++ SDK を `2022.11.0` に上げる
  - @melpon
- [UPDATE] libwebrtc を `m105.5195.0.0` に上げる
  - @melpon
- [UPDATE] Boost を `1.80.0` に上げる
  - @melpon
- [UPDATE] Android NDK を `r25b` に上げる
  - @melpon

## 2022.3.0

- [UPDATE] Sora C++ SDK を `2022.9.0` に上げる
  - @melpon
- [ADD] OnDataChannel を追加
  - @melpon
- [FIX] Sora Unity SDK のクライアント情報を設定するように追加
  - @torikizi
- [FIX] UnityAudioOutput = true で ADM を Stop しても再生中のままになっていたのを修正
  - @melpon
- [FIX] ProcessAudio が機能してなかったのを修正
  - @melpon

## 2022.2.1

- [CHANGE] OnAddTrack, OnRemoveTrack に connectionId を追加
  - @melpon
- [UPDATE] Sora C++ SDK を `2022.7.8` に上げる
  - @melpon

## 2022.2.0

- [CHANGE] Multistream, Spotlight, Simulcast を Nullable にする
  - @melpon
- [CHANGE] macOS x86_64 の対応をドロップ
  - @melpon
- [UPDATE] Sora C++ SDK 化
  - @melpon
- [UPDATE] Sora C++ SDK を `2022.7.5` に上げる
  - @melpon
- [UPDATE] libwebrtc を `103.5060.5.0` に上げる
  - @melpon
- [ADD] macOS M1 に対応
  - @melpon
- [ADD] Android の OpenGL ES を使った Unity カメラキャプチャに対応
  - @melpon
- [ADD] Ubuntu に対応
  - @melpon
- [ADD] BundleId 追加
  - @melpon
- [ADD] HTTP Proxy に対応
  - @melpon

## 2022.1.0

- [CHANGE] Github Actions の `macos-latest` を `macos-11` へ変更する
  - @voluntas
- [CHANGE] Github Actions の `windows-latest` を `windows-2019` へ変更する
  - @melpon
- [UPDATE] libwebrtc を `99.4844.1.0` に上げる
  - @voluntas @melpon @torikizi
- [ADD] `Insecure`, `Video`, `Audio` フラグを追加
  - @melpon

## 2021.4.0

- [CHANGE] `data_channel_messaging` を `data_channles` へ変更する
  - @torikizi
- [UPDATE] protocol buffers を `3.19.1` に上げる
  - @voluntas
- [UPDATE] libwebrtc を `94.4606.3.4` に上げる
  - @melpon @torikizi @voluntas
- [UPDATE] Boost のバージョンを `1.78.0` に上げる
  - @voluntas
- [UPDATE] signaling mid 対応
  - @melpon
- [ADD] `Sora.Config.SignalingUrlCandidate` に複数のシグナリング URL 指定を可能にする
  - @melpon
- [ADD] `"type": "redirect"` でクラスター機能でのリダイレクトに対応

## 2021.3.1

- [FIX] Let's Encrypt な証明書の SSL 接続が失敗する問題を修正する
  - @melpon

## 2021.3

- [CHANGE] `Sora.Config` のフィールド名および `Sora.VideoCodec`, `Sora.AudioCodec` の名前を変更する
  - `VideoCodec` → `VideoCodecType`
  - `VideoBitrate` → `VideoBitRate`
  - `AudioCodec` → `AudioCodecType`
  - `AudioBitrate` → `AudioBitRate`
  - @melpon
- [CHANGE] `Sora.Connect` 関数の戻り値を `bool` から `void` に変更する
  - エラーハンドリングは `Sora.Connect` の戻り値ではなく `Sora.OnDisconnect` を利用する
  - @melpon
- [UPDATE] libwebrtc のバージョンを M92 (4515@{#9}) に上げる
  - @melpon
- [ADD] `simulcastRid` に対応する
  - @torikizi
- [ADD] `clientId` に対応し、 `Sora.config` に `ClientId` を追加する
  - @torikizi
- [ADD] `spotlight_focus_rid` と `spotlight_unfocus_rid` に対応し、 `Sora.config` に以下のフィールドを追加する
  - `SpotlightFocusRid`
  - `SpotlightUnfocusRid`
  - @torikizi
- [ADD] プッシュ通知に対応する
  - @melpon
- [ADD] DataChannel を使ったメッセージングに対応しする
  - @melpon
- [ADD] `Sora.Config` に DataChannel メッセージング用の `List<DataChannelMessaging>` 型のフィールドを追加する
  - @melpon
- [ADD] `Sora.Config` に DataChannel メッセージング用のフィールドを追加する
  - `EnableDataChannelSignaling`
  - `DataChannelSignaling`
  - `DataChannelSignalingTimeout`
  - `EnableIgnoreDisconnectWebsocket`
  - `IgnoreDisconnectWebsocket`
  - `DisconnectWaitTimeout`
  - @melpon
- [ADD] WebSocket シグナリングの re-offer に対応する
  - @melpon
- [ADD] `Sora.Disconnect` 関数と `Sora.OnDisconnect` コールバックを追加する
  - @melpon
- [ADD] SRTP/SRTCP で AES-GCM 128/256 を利用可能にする
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
