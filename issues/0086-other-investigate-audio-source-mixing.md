# 複数音声ソースをミックスして送信できるか調査する

- Created: 2026-09-11
- Completed: {YYYY-MM-DD}
- Branch: feature/debug-audio-source-mixing
- Polished: 2026-09-11

## 目的

マイク入力とアプリ内で生成した音声（例: ゲームの音声）をアプリ側でミックスし、そのミックス済み音声を Sora に送信できるかを調査する。Sora Unity SDK 利用者から「複数の音声をミックスして送信したい」という問い合わせがあり、SDK が持つ任意音声入力の経路で実現できるのか、追加の SDK 対応が必要なのかを切り分ける必要があるため。

## 現状

- `Sora.Config.UnityAudioInput` を `true` にすると、録音デバイスの代わりに `Sora.ProcessAudio(float[] data, int offset, int samples)` に渡したデータを送信音声として利用できる。`Sora.ProcessAudio` は `src/sora.cpp` の `Sora::ProcessAudio` から `src/unity_audio_device.h` の `UnityAudioDevice::ProcessAudioData` を呼び、float のデータを int16 に変換して libwebrtc の `AudioDeviceBuffer` へ供給する。扱えるのは 48000Hz Stereo のデータのみで、`Sora.ProcessAudio` は第 3 引数のサンプル数をステレオとして 2 倍して渡す。
- `Sora::CreateADM` は `UnityAudioDevice::Create` に `adm_recording = !unity_audio_input` を渡す。`UnityAudioInput` が `true` のときは SDK 内蔵の録音デバイスが無効化されるため、SDK のマイク入力とアプリ生成音声を同時に扱うことはできず、ミックスはアプリ側で行う必要がある。
- iOS では `Sora::Connect` が `unity_audio_input` が `true` の場合に `IosAudioInit` を呼ばない。マイク入力をアプリ側で取得する場合の `AVAudioSession` の初期化・カテゴリ設定はアプリ側の責務になる。
- サンプル `SoraUnitySdkExamples/Assets/SoraSample.cs` の `Render` コルーチンは `AudioRenderer.Render` で Unity がスピーカーへ出力しようとする音を取得して `ProcessAudio` に渡す例のみで、マイク入力を含めてミックスする例はない。
- ドキュメント `sora-unity-sdk-doc` の `functions_audio.rst` の「Unity 音声入力を送信する場合」も `AudioRenderer` の単一ソース例のみで、複数ソースのミックス方法には触れていない。

## 設計方針

本 issue は調査と、その結果に基づく方法の整理までを範囲とする。既存の `UnityAudioInput` と `Sora.ProcessAudio` の経路は変更しないことを前提とする。

- `UnityAudioInput` を `true` にし、マイク入力とアプリ生成音声をアプリ側でミックスした 48000Hz Stereo の float データを `Sora.ProcessAudio` に渡す経路で送信できるかを確認する。
- マイク入力の取得方法（Unity の `Microphone` クラス、`AudioSource` の `OnAudioFilterRead` など）と、`AudioRenderer.Render` の出力とのミックス方法を確認する。
- iOS / Android でマイク入力と `UnityAudioInput` を併用する際のオーディオセッション・権限・サンプルレートの制約を確認する。
- ミックス処理を SDK 側で提供すべきか、アプリ側の実装とドキュメント・サンプルの整備で足りるかを切り分ける。

## 完了条件

- マイク入力とアプリ生成音声をミックスして Sora に送信できるかどうかの結論が出ていること。
- 実現可能な場合、ミックスの場所（アプリ側 / SDK 側）、必要なサンプルレートとチャンネル、iOS / Android の制約が整理されていること。
- SDK の機能追加が必要か、ドキュメント・サンプルの整備で足りるかの方針が決まっていること。
- ドキュメント・サンプルの整備が必要な場合は、それを扱う issue が別途作成されていること。
