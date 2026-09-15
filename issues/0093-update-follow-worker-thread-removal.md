# Sora C++ SDK の worker_thread 削除に追随する

- Created: 2026-09-15
- Completed: {YYYY-MM-DD}
- Branch: feature/update-follow-worker-thread-removal
- Polished: {YYYY-MM-DD}

## 目的

libwebrtc の issue 558821261「Deprecate and remove PeerConnectionFactoryDependencies::worker_thread」で worker thread が廃止される。これに対応して Sora C++ SDK は、専用 worker thread の生成をやめて network thread を使う変更 (方針1) と、`SoraClientContext::worker_thread()` の削除および `dependencies.worker_thread` を渡さない変更 (方針2) を行う。

Sora Unity SDK は sora-cpp-sdk の `SoraClientContext` をプリビルドまたはソースビルドで取り込み、`configure_dependencies` で `dependencies.worker_thread` を参照している。方針2 で sora-cpp-sdk が `dependencies.worker_thread` を設定しなくなると、この参照が nullptr 参照になってクラッシュする。さらに削除系 CL を含む libwebrtc に上がると `dependencies.worker_thread` と `SoraClientContext::worker_thread()` が無くなり、ビルドできなくなる。本 issue では sora-cpp-sdk の変更に追随し、Sora Unity SDK から worker_thread への参照を無くす。

## 現状

- worker thread は独自に生成しておらず、sora-cpp-sdk の `SoraClientContext` から借りている
- `src/sora.cpp` の `SoraClientConfig::configure_dependencies` に設定するラムダ内で `dependencies.worker_thread` を 5 箇所で使っている
  - worker thread 上の JNIEnv の取得 (`sora::GetJNIEnv`)
  - Android の application context の取得 (`sora_unity_sdk::GetAndroidApplicationContext`)
  - `Sora::CreateADM` への受け渡し
  - `dependencies.adm` の設定
  - `DeleteLocalRef` によるローカル参照の解放
- `sora_context_->worker_thread()` を 2 箇所 (`Sora::SetSpeakerVolume` / `Sora::SetMicrophoneVolume`) で使い、ADM の音量設定を worker thread 上で実行している
- `src/sora.h` の `Sora::CreateADM` は `webrtc::Thread*` の引数 (`worker_thread`) を取り、ADM と `UnityAudioDevice` の生成をそのスレッド上で実行している
- `DEPS` は `SORA_CPP_SDK_VERSION=2026.2.1` / `WEBRTC_BUILD_VERSION=m150.7871.3.1` を指している

## 前提条件

- sora-cpp-sdk の worker_thread 削除 (`SoraClientContext::worker_thread()` の削除と `dependencies.worker_thread` を渡さない変更) を含むバージョンがリリースされていること。本 issue では `DEPS` の `SORA_CPP_SDK_VERSION` をそのバージョンに更新する
- 現時点ではそのバージョンは存在しないため着手できない

## 設計方針

- `dependencies.worker_thread` の参照を `dependencies.network_thread` に置き換える
- `sora_context_->worker_thread()` の呼び出しを network thread のアクセサ (`SoraClientContext::network_thread()`) に置き換える
- `src/sora.h` の `Sora::CreateADM` が取る `webrtc::Thread*` の引数を network thread に置き換える
- worker thread 上で行っていた ADM と `UnityAudioDevice` の生成・アクセスは network thread 上で行う
- `DEPS` の `SORA_CPP_SDK_VERSION` を worker_thread 削除を含むバージョンに更新する

## 完了条件

- Sora Unity SDK に worker_thread 関連の参照が 0 件になっていること
- Unity の各プラットフォーム (Windows / macOS / Android / iOS) でビルドが通ること
- 各プラットフォームで Sora に実接続して動作確認できていること
- `CHANGES.md` の `## develop` に変更内容を追記していること
