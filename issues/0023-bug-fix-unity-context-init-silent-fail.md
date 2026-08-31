# UnityContext::Init のログ初期化失敗時に SDK が沈黙初期化不能になる問題を修正する

- Priority: High
- Created: 2026-08-27
- Branch: feature/fix-unity-context-init-silent-fail
- Polished: 2026-08-31
- Milestone: 2026.2.0

## 目的

`UnityContext::Init` はログファイル初期化に失敗すると `ifs_` の設定や `OnGraphicsDeviceEvent` の呼び出しに到達せず、以降 `IsInitialized()` が永久に false を返す。ユーザーには `sora_create` が nullptr を返し続けるだけで、原因不明の初期化不能状態になる。macOS の `.app` 起動時 (CWD が `/` 等の書き込み不可ディレクトリになる)、Windows の `Program Files` 配下など、カレントディレクトリが読み取り専用になる環境で発火するため、リリース前に必ず対応する。

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

ログ出力先はハードコードの `"./"` (カレントディレクトリ)。macOS の `.app` 起動時や Windows の `Program Files` 配下のようにカレントディレクトリに write 権限がない read-only 環境では `log_sink_->Init()` が失敗する。

## 設計方針

- `log_sink_->Init()` に失敗しても `ifs_ = ifs;` と `OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize)` を続行する
  - ログが吐けないだけで SDK の主要機能は動作可能
  - 失敗時の `return;` をやめ、ログ初期化の失敗を記録して先に進む
- ログ出力先ハードコード `"./"` を見直し、書き込み可能なディレクトリへ次の順でフォールバックする
  - 環境変数 `SORA_UNITY_SDK_LOG_DIR` が設定されていれば、そのパスを最優先で使う
  - 未設定なら `"./"` (カレントディレクトリ) を使う
  - 環境変数と `"./"` の双方で失敗した場合に、プラットフォーム別のユーザーデータディレクトリへフォールバックする
    - macOS: ホームディレクトリ配下、Windows: `%LOCALAPPDATA%` 配下、Ubuntu: `$XDG_DATA_HOME` 配下
  - すべて失敗した場合はログ出力を無効化したまま初期化を続行する
- ログ失敗フラグは `UnityContext` のメンバ変数 (例: `log_initialized_`) として追加し、`FileRotatingLogSink::Init()` が成功した場合のみ true にする
  - `Shutdown()` はフラグを確認し、true の場合のみ `RemoveLogToStream` と `log_sink_.reset()` を実行する
- ログ初期化失敗時は `webrtc::LogMessage::LogToDebug` の設定値を変更し、`RTC_LOG` のエラーが stderr に出力されるようにする
  - ログファイルが吐けなくとも Unity Editor のログ (Editor.log / Console) から状況を把握できるようにするため
  - 現状は `LogToDebug(LS_NONE)` に設定されており、失敗経路では `AddLogToStream` にも到達しないため、失敗ログはどこにも出力されない
- C# 側にログ失敗を伝える API の追加は本 issue の対象外とする
  - ユーザーへの通知は本 issue 内で実現する Unity Editor へのログ出力で行う
  - C# 側の明示的な通知手段が必要なら別 issue で設計する

## 完了条件

- read-only working dir でも `sora_create()` が動作する
- ログ初期化失敗時に `IsInitialized()` が false のまま停止することがない
- ログ出力先が read-only 環境に耐えるパスにフォールバックする
- ログ初期化失敗時に Unity Editor のログへエラーが出力され、ユーザーが状況を把握できる
- macOS / Windows の書き込み不可ディレクトリで起動した場合と、Unity Editor Play で SDK が正しく動作することを確認する
- `CHANGES.md` の `## develop` に `[FIX] UnityContext::Init のログ初期化失敗時に SDK が沈黙初期化不能になる問題を修正する` を追記する
