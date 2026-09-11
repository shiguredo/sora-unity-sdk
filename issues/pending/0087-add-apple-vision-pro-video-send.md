# Apple Vision Pro で映像送信に対応する

- Created: 2026-09-11
- Completed: {YYYY-MM-DD}
- Branch: feature/add-apple-vision-pro-video-send
- Polished: {YYYY-MM-DD}

## 目的

Apple Vision Pro (visionOS) 向けの対応は映像受信のみで、Unity Camera の映像を送信できない。ビデオ通話アプリとして送受信の両方ができることが重要であり、Apple の Persona アバターと Unity Camera の映像を送信できると、標準のメッセージアプリより便利に使える可能性があるため、映像送信に対応する。

## 現状

- develop には visionOS 向けの実装は含まれておらず、対応は `support/apple-vision-pro` ブランチで進めている。
- 現状の visionOS 対応は映像受信のみで、カメラデバイスと映像送信は無効化されている。
- 映像送信を有効にするには webrtc-build / sora-cpp-sdk / sora-unity-sdk の 3 リポジトリに手を入れる必要がある。
  - webrtc-build: `visionos.patch` で無効化しているカメラデバイスと映像送信の部分を復活させる。
  - sora-cpp-sdk: `camera_device_capturer.cpp` で mac_capturer を無効化している if を削除する。
  - sora-unity-sdk: video capture を復活させ、Persona を取得できる仕組みに切り替える。
- ローカルの仮実装で映像送信の動作確認まではできているが、sora-unity-sdk に入れた修正の一部が本来 sora-cpp-sdk の native 側に入れるべき処理に見えており、この懸念が正しいかは未確認である。

## 設計方針

- 対応範囲は Unity Camera の映像送信と Persona アバターの映像送信とする。
- 可能であれば、エンタープライズアカウントでのみ利用できる Vision Pro のカメラデバイスの利用まで対応する。
- リポジトリ間の責務を分離し、sora-unity-sdk 側には Unity 固有の処理だけを残す。仮実装が native 側に置くべき処理を含んでいないかを sora-cpp-sdk の責務と照らし合わせて整理する。

## 完了条件

- `support/apple-vision-pro` ブランチで Unity Camera の映像を送信できる。
- Persona アバターの映像を送信できる。
- 映像の送受信を同時に行える。
- webrtc-build / sora-cpp-sdk / sora-unity-sdk のどこにどの変更が必要かが整理されている。

## pending にした理由

2026-09-11 に pending にする。

- webrtc-build と sora-cpp-sdk の変更が前提であり、sora-unity-sdk 単独では完了できない。
- sora-unity-sdk の仮実装が本来 native 側にあるべき処理を含む疑いがあり、リポジトリ間の責務分離を決める必要がある。
- Persona の取得方法と、エンタープライズ用途のカメラデバイス利用可否が未確定である。
- Apple Vision Pro 実機での動作確認と Persona の扱いの調査が終わるまでは実装方針を確定できない。
