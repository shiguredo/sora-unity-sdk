# モバイルデバイスのカメラとマイクの利用許諾を SDK から制御できるようにする

- Created: 2026-09-10
- Completed: {YYYY-MM-DD}
- Branch: feature/add-mobile-camera-mic-permission-control
- Polished: {YYYY-MM-DD}
- Reporter: @torikizi

## 目的

Android / iOS 実機でカメラとマイクを利用するときの利用許諾を、SDK 利用者が任意のタイミングで要求・確認できるようにし、拒否された場合は SDK 利用者へ通知できるようにする。現状は OS 任せで、カメラ / マイクを開くタイミングで利用許諾ダイアログが出るだけであり、起動後に許諾状態を確認したり、許諾が必要な操作の直前に改めて要求したりする手段が SDK にない。

## 現状

- `SoraUnitySdkExamples/Assets/SoraUnitySdk/Sora.cs` の `Sora.Config` には `Video` / `Audio` / `NoVideoDevice` / `NoAudioDevice` があるが、これらはデバイスを掴むかどうかの設定であり、利用許諾の確認・要求は行わない。
- Android / iOS ともに、カメラ / マイクの利用許諾はデバイスを開く時に OS がダイアログを出す処理に任せている。`src/mac_helper/ios_audio_init.mm` の `IosAudioInit` は `RTCAudioSession` の `initializeInput` で入力デバイスを初期化するのみで、利用許諾の状態確認や明示的な要求は行っていない。
- Android のオーディオデバイス制御は `Sora.IAudioOutputHelper` / `AudioOutputHelperFactory` と `Sora.aar` の `SoraAudioManagerFactory` で実装しているが、利用許諾は扱っていない。
- 以前のハンズフリー対応では `UnityPlayerActivity` を継承した Activity とカスタム `AndroidManifest.xml` でパーミッションを取得していたが、Bluetooth のパーミッションが不要になった時点でこの Activity は廃止されており、現在のリポジトリにパーミッション取得の実装は残っていない。
- 付近のデバイス (Bluetooth) のパーミッション要求は不要になったため、本 issue で対象とするのはカメラとマイクの利用許諾に限る。

## 設計方針

- SDK 利用者が利用許諾を取るためのヘルパー API を提供する。カメラ / マイクそれぞれについて状態確認と要求を行える形にする。
- 利用許諾が未取得の状態でカメラ / マイクを利用する処理が始まる場合は、利用許諾ダイアログを表示する。
- 利用許諾が拒否された場合は SDK 利用者へ通知する。拒否後に再度要求しても OS がダイアログを表示しない場合の扱いも決める。
- Android と iOS で API の形を揃え、既存の `Sora.Config` や `IAudioOutputHelper` の挙動は変えない。

## 完了条件

- Android / iOS 実機で、SDK 利用者が任意のタイミングでカメラ / マイクの利用許諾を要求でき、状態を確認できる
- 利用許諾が拒否された場合に SDK 利用者へ通知される
- 未許諾の状態でカメラ / マイクの利用を開始した場合に利用許諾ダイアログが表示される
- Windows / macOS を含む各ターゲットで既存の挙動が変わらない
- 各ターゲットのビルドが通る
- `CHANGES.md` の `## develop` に `[ADD]` エントリを追記する

## pending にした理由

2026-09-10 に pending にする。

- カメラ / マイクの利用許諾 API は Android のランタイムパーミッションと iOS の AVFoundation で仕組みが異なり、状態確認・要求・拒否通知を C# の共通 API としてどうまとめるかが未確定である。
- 実装を C# 側 (`Sora.cs`) に置くかネイティブ側 (`src/`) に置くかを含め、プラットフォームごとの実装方法を決める必要がある。
- 旧実装で使っていた `UnityPlayerActivity` を継承した Activity とカスタム `AndroidManifest.xml` は廃止済みであり、Activity の継承や Manifest のカスタムを利用者に求めない方法を改めて検討する必要がある。
- 拒否後の再要求は OS の仕様上ダイアログが再表示されない場合があり、設定画面への誘導や拒否状態の通知方法を決める必要がある。
- これらの設計が決まるまで実装方針を確定できないため保留とする。対応を再開するときは reopened にしてから、API の形とプラットフォームごとの実装方法を確定する。
