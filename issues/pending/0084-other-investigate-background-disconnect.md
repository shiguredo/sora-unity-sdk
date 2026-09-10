# Android / iOS でアプリがバックグラウンドに移ると Sora との接続が切断される問題を調査する

- Created: 2026-09-10
- Completed: {YYYY-MM-DD}
- Branch: feature/debug-investigate-background-disconnect
- Polished: {YYYY-MM-DD}

## 目的

Android / iOS の Unity アプリをバックグラウンドに移して復帰させた時に、Sora との接続が切断されてしまう問題について、再現条件と原因を調査し、SDK として対応すべき範囲を決める。SDK 利用者から「アプリがバックグラウンドに回った時に接続を維持する機能はあるか」という問い合わせがあり、アプリ側の実装で解決する問題か、SDK の機能として提供する問題かを切り分ける必要がある。

## 現状

- Android 実機で、アプリをバックグラウンドに放置するとアプリがフリーズし、復帰時に Activity が再起動する。その際に libwebrtc の DataChannel の接続が破棄され、WebSocket が `Software caused connection abort` で切断され、IceConnectionState が `connected` → `failed` → `disconnected` に変化して切断される、という報告がある。
- `SoraUnitySdkExamples/ProjectSettings/ProjectSettings.asset` の Player 設定は `runInBackground: 1` だが `iOSBackgroundModes: 0` と `iosUseCustomAppBackgroundBehavior: 0` であり、iOS の Background Modes は有効になっていない。
- `src/mac_helper/ios_audio_init.mm` の `IosAudioInit` は `RTCAudioSessionConfiguration` に `AVAudioSessionCategoryPlayAndRecord` を設定して入力デバイスを初期化するのみで、バックグラウンドへの移行やオーディオ割り込みからの復帰を扱う処理を持たない。
- `SoraUnitySdkExamples/Assets/SoraSample.cs` は `OnApplicationQuit` で `DisposeSora` を呼ぶのみで、`OnApplicationPause` / `OnApplicationFocus` を使った接続維持や再接続の処理を持たない。
- `SoraUnitySdkExamples/Assets/SoraUnitySdk/Sora.cs` の `AndroidAudioOutputHelper` は `SoraAudioManagerFactory` で音声経路を制御するが、フォアグラウンドサービスや常駐通知を持たない。
- Android / iOS のネイティブ実装にも、CallKit やバックグラウンドオーディオを使った常駐処理は無い。
- Google Meet / Zoom などでは、バックグラウンドに移行しても小窓や通話 UI で通話が継続している。PiP、CallKit、バックグラウンドオーディオ、フォアグラウンドサービスなどを利用していると考えられるが、確認はできていない。
- 切断がアプリ側の設定・実装の問題なのか、SDK の機能不足の問題なのかは切り分けられていない。

## 調査方針

- Android / iOS 実機のサンプルアプリで、バックグラウンドへの移行と復帰の前後における IceConnectionState、WebSocket、オーディオセッションのログを取得し、切断に至る流れを特定する。
- Unity の Player 設定 (`runInBackground` / iOS Background Modes)、アプリ側の実装 (Android のフォアグラウンドサービス、iOS の AVAudioSession / CallKit)、SDK 側の実装 (オーディオセッション管理、切断検知、再接続) に分けて影響を切り分ける。
- 他社製アプリがバックグラウンドでの通話継続に利用している仕組みを調査し、SDK が提供すべき機能と、利用者がアプリ側で実装すべき範囲を整理する。
- バックグラウンドでのカメラ・マイク利用は OS の制限を受けるため、音声のみ継続する場合、映像を停止する場合、復帰後に自動再接続する場合のそれぞれについて実現可能性を確認する。

## 完了条件

- Android / iOS 実機で、バックグラウンド移行・復帰時に何が起きて切断されるのかをログで説明できる
- 切断の原因がアプリ側の設定・実装にあるのか、SDK 側の機能不足にあるのかが明確になっている
- SDK として対応する範囲と、利用者がアプリ側で対応すべき範囲が決まっている
- 対応が必要な場合は、その実装を扱う issue が別途作成されている

## pending にした理由

2026-09-10 に pending にする。

- 原因の切り分けには Android / iOS 実機での再現確認が先に必要であり、確認する前に実装方針を決められない。
- バックグラウンドでの接続維持は、Android のフォアグラウンドサービスと、iOS の Background Modes / AVAudioSession / CallKit で仕組みが大きく異なる。SDK がどこまで提供するかの設計判断が必要である。
- OS のポリシーにより、バックグラウンドでのカメラ・マイクの利用は制限される。音声のみ継続するのか、映像も継続するのか、復帰後に再接続するのかといった仕様を決める必要がある。
- これらの調査と設計が終わるまでは実装に着手できないため保留とする。対応を再開するときは reopened にしてから、調査結果と対応方針を確定する。
