# unity_audio_device.h の英語コメントを日本語に翻訳する

- Priority: High
- Created: 2026-08-27
- Branch: fmt/translate-audio-device-comments
- Polished: {YYYY-MM-DD}
- Milestone: 2026.2.0

## 目的

`src/unity_audio_device.h` に残っている 15 箇所以上の英語コメントと、`src/unity_camera_capturer_d3d12.cpp` 先頭の英語コメントを日本語に翻訳する。AGENTS.md の「コメントは全て日本語にすること」規約に沿った状態に揃える。

## 現状

`src/unity_audio_device.h` の `UnityAudioDevice` は webrtc `AudioDeviceModule` を override する形で書かれており、interface 由来のセクションヘッダコメントが英語のまま残っている。代表例:

- `//opus supports up to 48khz sample rate, enforce 48khz here for quality`（コメント記号の直後にスペースが無いスタイル違反も併存）
- `//webrtc::AudioDeviceModule`
- `// Retrieve the currently utilized audio layer`
- `// Full-duplex transportation of PCM audio`
- `// Main initialization and termination`
- `// Device enumeration`
- `// Device selection`
- `// Audio transport initialization`
- `// Audio transport control`
- `// Audio mixer initialization`
- `// Speaker volume controls`
- `// Microphone volume controls`
- `// Speaker mute control`
- `// Microphone mute control`
- `// Stereo support`
- `// Playout delay`
- `// Only supported on Android.`
- `// Enables the built-in audio effects. Only supported on Android.`
- `// Only supported on iOS.`

さらに `src/unity_camera_capturer_d3d12.cpp` の先頭には `// D3D12 implementation of UnityCameraCapturer similar to D3D11 version` という英語コメントが残っている。

## 設計方針

- webrtc の interface 由来の英語セクションヘッダをすべて日本語に翻訳する
- コメント記号の直後にスペースを入れるスタイルに揃える
- 訳文は原文の意味を保ちつつ日本語として自然な表現にする
- 単純翻訳が難しい webrtc 用語（例: AudioTransport / mixer initialization）はカタカナや原語のまま残し、意味を補足する

## 完了条件

- `src/unity_audio_device.h` に英語コメントが残っていない
- `src/unity_camera_capturer_d3d12.cpp` 先頭のコメントが日本語である
- AGENTS.md の「コメントは全て日本語にすること」に反する箇所が該当ファイル内に残っていない
