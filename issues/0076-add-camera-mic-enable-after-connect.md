# カメラ/マイクを掴まずに接続してあとから有効化できるようにする

- Created: 2026-09-10
- Completed: {YYYY-MM-DD}
- Branch: feature/add-camera-mic-enable-after-connect
- Polished: {YYYY-MM-DD}
- Reporter: @torikizi

## 目的

音声・映像を有効にしたシグナリング (audio:true, video:true) で接続しつつ、開始時はカメラやマイクのデバイスを掴まず送信も行わず、接続を維持したままユーザーの操作であとから有効化できるようにする。会議アプリなどで入室時にカメラ・マイクをオフにしておき、必要になった時点でオンにするユースケースに対応するため。

## 現状

- `Sora.Config` には `NoVideoDevice` / `NoAudioDevice` があり、接続時にデバイスを掴まないようにできる。`NoAudioDevice` を true にするとダミーの ADM が使われる (`Sora::CreateADM`)。
- 実行時のミュートは `Sora.AudioEnabled` / `Sora.VideoEnabled` として実装済み。これは `AudioTrackInterface::set_enabled` / `VideoTrackInterface::set_enabled` を切り替えて送信を止めるソフトウェアミュートであり、デバイスは掴んだままになる。
- 接続後のカメラ切り替えは `Sora.SwitchCamera` がある。`SwitchCamera` の remarks は `NoVideoDevice = false` で接続した場合を想定しており、`NoVideoDevice = true` で接続した場合の動作は保証されていない。`Sora::DoSwitchCamera` には `video_track_` が null の場合に `AddTrack` する分岐がある。
- マイクは接続後にデバイスを有効化・無効化する API が無い。`UnityAudioDevice` は `MicrophoneMuteIsAvailable` / `SetMicrophoneMute` を実装しているが、`Sora` クラスおよび C# 側には公開されていない。
- ハードウェアミュート対応の雨傘 issue では iOS / Android は対応済みで、Unity が残っている。

## 設計方針

- 接続時にデバイスを掴まず、接続後にカメラ・マイクを掴んで有効化する API を追加する。既存の `NoVideoDevice` / `NoAudioDevice` での接続を起点にするか、新たな `Sora.Config` を設けるかは実装時に判断する。
- シグナリングは audio:true, video:true を維持し、デバイスを掴んでいない間は送信しない。
- 有効化・無効化は接続を維持したまま行えるようにし、無効化時はデバイスを解放する。
- 映像は既存の `Sora::DoSwitchCamera` の `AddTrack` 経路を活用できるか検討する。
- マイクは ADM ごと差し替えるのではなく、`UnityAudioDevice` の `SetMicrophoneMute` など Sora C++ SDK が提供するデバイス制御 API を利用できるか調査する。必要であれば Sora C++ SDK 側の対応も検討する。
- ソフトウェアミュート (`Sora.AudioEnabled` / `Sora.VideoEnabled`) との役割分担を整理する。

## 完了条件

- audio:true, video:true のシグナリングで、カメラ・マイクのデバイスを掴まずに接続できる。
- 接続を維持したまま、あとからカメラ・マイクを有効化できる。
- 有効化したあとで再度無効化し、デバイスを解放できる。
- サンプルでこの一連の操作を確認できる。
