# UnityAudioDevice の EnableBuiltInAEC/AGC/NS の戻り値を修正する

- Priority: High
- Created: 2026-08-27
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
