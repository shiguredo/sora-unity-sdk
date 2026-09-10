# UnityAudioDevice の EnableBuiltInAEC/AGC/NS の戻り値を修正する

- Priority: High
- Created: 2026-08-27
- Completed: 2026-09-10
- Branch: fix/enable-builtin-audio-effects-return
- Polished: {YYYY-MM-DD}
- Milestone: 2026.2.0

## 目的

`src/unity_audio_device.h` の `EnableBuiltInAEC` / `EnableBuiltInAGC` / `EnableBuiltInNS` が実装上は何もしないのに `return 0`（成功）を返している。呼び出し側は「Built-in 音声処理の有効化に成功した」と誤解し、実際には効果が無いのに設定済みとして扱ってしまう。webrtc `AudioDeviceModule` interface の契約通り非対応であることを返り値で表現する。

## 現状

`src/unity_audio_device.h` の `UnityAudioDevice` は `webrtc::AudioDeviceModule` を override しており、以下 3 メソッドが実装されている。

- `EnableBuiltInAEC(bool enable)`
- `EnableBuiltInAGC(bool enable)`
- `EnableBuiltInNS(bool enable)`

いずれも Only supported on Android 相当のコメントが付き、本体では特に処理を行わず `return 0;` を返している。webrtc の interface 契約では、非対応時は負値または `-1` を返すのが慣習であり、`0` は「成功」を意味する。呼び出し側が有効化の可否を判定できず、後段の処理が破綻する。

## 設計方針

- 非対応プラットフォームでは `return -1;` を返すように書き換える
- Android 実装が可能なら、内部 `adm_` に処理を委譲する
- `adm_->BuiltInAECIsAvailable` / `BuiltInAGCIsAvailable` / `BuiltInNSIsAvailable` の結果を参照して分岐する形も検討する

## 完了条件

- `EnableBuiltInAEC` / `EnableBuiltInAGC` / `EnableBuiltInNS` が非対応時に負値を返す
- 呼び出し側で有効化失敗を検知できる
- webrtc の他の `AudioDeviceModule` 実装との整合が取れている
- `CHANGES.md` の `## develop` に `[FIX]` を追記する

## 解決方法

コード変更は行わず closed にした。issue の前提が、DEPS が固定する libwebrtc m150 (branch-heads/7871) の一次資料およびこのリポジトリの現行実装と矛盾し、報告されているバグが現行実装には存在しないため。

一次資料での照合結果:

- `api/audio/audio_device.h` に戻り値の契約は存在しない。`EnableBuiltInAEC` 等の宣言は「Enables the built-in audio effects. Only supported on Android.」というコメントのみで、非対応時に何を返すべきかは定めていない (`modules/audio_device/g3doc/audio_device_module.md` にも記述なし)。
- 上流実装は不統一であり、「非対応時は -1」を返すのは `AudioDeviceGeneric` の既定実装のみ (`modules/audio_device/audio_device_generic.cc`)。この SDK が Windows で使う `webrtc_win::WindowsAudioDeviceModule` は `BuiltInAECIsAvailable` 等が false のまま `EnableBuiltInAEC` / `EnableBuiltInAGC` / `EnableBuiltInNS` が `return 0` しており (`modules/audio_device/win/audio_device_module_win.cc`)、本 issue が「誤り」とする挙動と同一。
- 「呼び出し側が有効化の可否を判定できず、後段の処理が破綻する」は成立しない。実際の呼び出し系統は `cricket::WebRtcVoiceEngine::Init` → `ApplyOptions` (`media/engine/webrtc_voice_engine.cc`) で、`BuiltInAECIsAvailable()` / `BuiltInAGCIsAvailable()` / `BuiltInNSIsAvailable()` が true のときだけ `EnableBuiltIn*` を呼ぶ。本クラスは 3 メソッドとも false を返す (`src/unity_audio_device.h`) ため、`EnableBuiltIn*` は実行時に一度も呼ばれず、既定のソフトウェア EC / AGC / NS がそのまま有効になる。本リポジトリ内 (`src/`・`proto/`・`SoraUnitySdkExamples/`) にも `EnableBuiltIn*` の呼び出し元は存在しない。
- 設計方針の「Android 実装が可能なら内部 `adm_` に処理を委譲する」は成立しない。Android の `adm_` は `webrtc::jni::AndroidAudioDeviceModule` (`sdk/android/src/jni/audio_device/audio_device_module.cc`) だが、`EnableBuiltInAGC` は `RTC_CHECK_NOTREACHED()` で必ずプロセスが落ち、`BuiltInAGCIsAvailable` は「Not implemented for any input device on Android.」とコメントされた常に false の実装。`EnableBuiltInAEC` / `EnableBuiltInNS` も非対応時に負値を返さず `RTC_CHECK(BuiltInAECIsAvailable())` でクラッシュする。さらに Unity 音声入力 (`unity_audio_input`) 時は録音経路が `adm_` ではなく Unity 側の `ProcessAudioData` 経由 (`src/sora.cpp`) なので、仮に委譲して IsAvailable が true になっても、voice engine がソフトウェア EC を無効化する一方でハードウェア AEC は実際の捕捉音声に掛からず、音声品質が劣化する。
- 完了条件「webrtc の他の `AudioDeviceModule` 実装との整合が取れている」は判定不能。上流実装は Windows が 0、`AudioDeviceGeneric` 系が -1、Android がクラッシュと互いに食い違っており、整合先が定まらない。

以上より、現行実装 (非対応を `BuiltIn*IsAvailable()` の false と `EnableBuiltIn*` の `return 0` で表現) は、実呼び出し元である webrtc voice engine の呼び出し規律 (IsAvailable でゲート) に沿っており、`WindowsAudioDeviceModule` とも同一の挙動。バグ修正 issue として成立しないため closed にする。

備考: polish-issue のレビューで本件が処理不能指摘 (issue の前提が一次資料・現行実装と矛盾) として確定したため、`Polished:` は更新していない。
