# UnityContext::Init のログ初期化失敗時に SDK が沈黙初期化不能になる問題を修正する

- Priority: High
- Created: 2026-08-27
- Branch: fix/unity-context-init-silent-fail
- Polished: {YYYY-MM-DD}
- Milestone: 2026.2.0

## 目的

`UnityContext::Init` はログファイル初期化に失敗すると `ifs_` の設定や `OnGraphicsDeviceEvent` の呼び出しに到達せず、以降 `IsInitialized()` が永久に false を返す。ユーザーには `sora_create` が nullptr を返し続けるだけで、原因不明の初期化不能状態になる。macOS の `.app` 内部 (quarantine 下)、Windows の `Program Files` 配下など read-only 環境で確実に発火するため、リリース前に必ず対応する。

## 現状

`src/unity_context.cpp` の `UnityContext::Init` は次のように動作する:

```cpp
void UnityContext::Init(IUnityInterfaces* ifs) {
  std::lock_guard<std::mutex> guard(mutex_);

#if defined(SORA_UNITY_SDK_WINDOWS) || defined(SORA_UNITY_SDK_MACOS) || \
    defined(SORA_UNITY_SDK_UBUNTU)
  ...
  log_sink_.reset(new webrtc::FileRotatingLogSink("./", "webrtc_logs",
                                                  kDefaultMaxLogFileSize, 10));
  if (!log_sink_->Init()) {
    RTC_LOG(LS_ERROR) << __FUNCTION__ << ": Failed to open log file";
    log_sink_.reset();
    return;
  }
  ...
#endif

  ifs_ = ifs;
  OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);
}
```

`log_sink_->Init()` が失敗すると `return;` してしまい、その先の `ifs_ = ifs;` と `OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize)` に到達しない。

`IsInitialized()` は次の条件で判定する:

```cpp
if (ifs_ == nullptr || graphics_ == nullptr) {
  return false;
}
return true;
```

`ifs_` が未設定のままなので `IsInitialized()` は永久に false を返し、`sora_create()` は毎回 nullptr を返す。C# 側にはエラー内容が届かない (`sora_create` は失敗理由を返さない)。

ログ出力先はハードコードの `"./"` (カレントディレクトリ)。macOS の `.app` 内部、Windows の `Program Files` 配下、Unity Editor の実行環境で write 権限がない場合など、read-only 環境では確実に失敗する。

## 設計方針

- `log_sink_->Init()` 失敗でも `ifs_ = ifs;` と `OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize)` は続行する
  - ログが吐けないだけで SDK の主要機能は動作可能
  - `log_sink_.reset();` の後 `return;` する代わりに、ログ失敗フラグを立てて先に進む
- ログ出力先ハードコード `"./"` を見直す
  - プラットフォームごとに適切なユーザーデータディレクトリを使う (macOS の `Application Support`、Windows の `%LOCALAPPDATA%` など)
  - または環境変数 (例: `SORA_UNITY_SDK_LOG_DIR`) で override 可能にする
- C# 側にログ失敗を伝える手段を追加する (別 issue で C ABI の null チェックと併せて設計する)
- ログ失敗時の RTC_LOG は `Debug` などにも出力し、ログが吐けなくとも Unity Console から状況把握できるようにする

## 完了条件

- read-only working dir でも `sora_create()` が動作する
- ログ初期化失敗時に `IsInitialized()` が false のまま停止することがない
- ログ出力先が read-only 環境に耐えるパスにフォールバックする
- macOS の `.app` 配布、Windows の `Program Files` 配下、Unity Editor Play で SDK が正しく動作することを確認する
- `CHANGES.md` の `## develop` に `[FIX] UnityContext::Init のログ初期化失敗時に SDK が沈黙初期化不能になる問題を修正する` を追記する
