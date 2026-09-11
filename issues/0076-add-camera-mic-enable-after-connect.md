# カメラ/マイクを掴まずに接続してあとから有効化できるようにする

- Created: 2026-09-10
- Completed: {YYYY-MM-DD}
- Branch: feature/add-camera-mic-enable-after-connect
- Polished: 2026-09-11
- Reporter: @torikizi

## 目的

音声・映像を有効にしたシグナリング (audio:true, video:true) で接続しつつ、開始時はカメラやマイクのデバイスを掴まず送信も行わず、接続を維持したままユーザーの操作であとから有効化できるようにする。会議アプリなどで入室時にカメラ・マイクをオフにしておき、必要になった時点でオンにするユースケースに対応するため。

## 現状

- `Sora.Config` には `NoVideoDevice` / `NoAudioDevice` があり、接続時にデバイスを掴まないようにできる。`NoAudioDevice` を true にするとダミーの ADM が使われる (`Sora::CreateADM`)。ただし iOS では `Sora::Connect` が `role` が `sendonly` / `sendrecv` かつ `UnityAudioInput = false` の場合に `IosAudioInit` (`AVAudioSession` の入力初期化) を必ず呼ぶため、`NoAudioDevice = true` でも入力デバイスの初期化が走る。
- 実行時のミュートは `Sora.AudioEnabled` / `Sora.VideoEnabled` として実装済み。これは `AudioTrackInterface::set_enabled` / `VideoTrackInterface::set_enabled` を切り替えて送信を止めるソフトウェアミュートであり、デバイスは掴んだままになる。
- 接続後のカメラ切り替えは `Sora.SwitchCamera` がある。`SwitchCamera` の remarks は `NoVideoDevice = false` で接続した場合を想定しており、`NoVideoDevice = true` で接続した場合の動作は保証されていない。`Sora::DoSwitchCamera` には `video_track_` が null の場合に `AddTrack` する分岐がある。
- マイクは接続後にデバイスを有効化・無効化する API が無い。`UnityAudioDevice` の `MicrophoneMuteIsAvailable` / `SetMicrophoneMute` は ADM に対するハードウェアミュートであり、デバイスを掴む・解放する操作ではなく、`Sora` クラスおよび C# 側には公開されていない。
- ハードウェアミュートの API は iOS SDK (`setVideoHardMute` など) と Android SDK (`setAudioHardMute` など) では提供されており、本 SDK では未対応である。

## 設計方針

- 接続時にデバイスを掴まず、接続後にカメラ・マイクを掴んで有効化する API を追加する。既存の `NoVideoDevice` / `NoAudioDevice` での接続を起点にするか、新たな `Sora.Config` を設けるかは実装時に判断する。
- シグナリングは audio:true, video:true を維持し、デバイスを掴んでいない間は送信しない。なお、現在の実装では音声トラックは `DoConnect` (`src/sora.cpp`) の送信 role (`sendonly` / `sendrecv`) で必ず作成され `OnSetOffer` で追加されるため、`NoAudioDevice = true` だけでは送信は止まらず、開始時にソフトウェアミュート (`AudioEnabled = false`) にするか、音声トラックを追加しない設計が必要になる。
- 有効化・無効化は接続を維持したまま行えるようにし、無効化時はデバイスを解放する。
- 映像は既存の `Sora::DoSwitchCamera` の `AddTrack` 経路を活用できるか検討する。ただし `AddTrack` は接続時に確立済みの SDP に映像 m-line を追加しないため、この経路だけでは送信されない可能性があり、接続時に映像 m-line を立てておく設計か、再交渉が必要になる点を確認する。
- マイクは ADM ごと差し替えるのではなく、Sora C++ SDK が提供する ADM の録音制御 (`StartRecording` / `StopRecording` など) を利用してデバイスの有効化・無効化ができるか調査する。なお ADM は `SoraClientContext` の生成時に固定されるため、接続中の差し替えはできない。必要であれば Sora C++ SDK 側の対応も検討する。
- ソフトウェアミュート (`Sora.AudioEnabled` / `Sora.VideoEnabled`) は送信停止のみでデバイスを掴み続けるのに対し、本機能はデバイスの掴み・解放を行う、という役割分担を整理する。

## 完了条件

- audio:true, video:true のシグナリングで、カメラ・マイクのデバイスを掴まずに接続でき、デバイスを掴んでいない間は音声・映像の送信を行わない。
- 接続を維持したまま、あとからカメラ・マイクを有効化できる。
- 有効化したあとで再度無効化し、デバイスを解放できる。
- サンプルでこの一連の操作を確認できる。
