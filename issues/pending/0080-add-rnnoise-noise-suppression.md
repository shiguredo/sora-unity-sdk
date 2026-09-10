# RNNoise を利用できるようにするライブラリを用意する

- Created: 2026-09-10
- Completed: {YYYY-MM-DD}
- Branch: feature/add-rnnoise-noise-suppression
- Polished: {YYYY-MM-DD}

## 目的

Unity アプリから送信する音声に、libwebrtc のソフトウェア NS とは別のノイズキャンセルを選択できるようにする。より強いノイズ除去を求める利用者向けに、RNNoise を利用できる仕組みを用意する。

## 現状

- Unity 側の音声入力は `SoraUnitySdkExamples/Assets/SoraUnitySdk/Sora.cs` の `Sora.ProcessAudio(float[] data, int offset, int samples)` で受け取る。`Sora.Config.UnityAudioInput = true` のとき `sora_process_audio` 経由でネイティブへ渡し、現在は 48000Hz ステレオの float データのみを受け取る。
- ネイティブ側は `src/sora.cpp` の `Sora::ProcessAudio` が `UnityAudioDevice::ProcessAudioData` を呼ぶ。`src/unity_audio_device.h` の `ProcessAudioData` は float を int16 へ変換し、`webrtc::AudioDeviceBuffer` へ 10ms (480 サンプル) 単位で投入する。
- ノイズ抑制は libwebrtc の AudioProcessing (APM) が既定で有効になっている。RNNoise のような外部のノイズサプレッションライブラリは組み込まれていない。
- `DEPS` が管理する外部依存は Sora C++ SDK / libwebrtc / Boost / protobuf などで、RNNoise は含まれていない。

## 設計方針

- RNNoise (xiph/rnnoise、BSD-3-Clause) を選択肢として組み込む。48kHz モノラル、480 サンプル固定フレーム、入力は float の [-32768, 32767] スケールという制約に合わせる。
- Unity のステレオ入力をモノラルへダウンミックスし、RNNoise 適用後にステレオへ戻す経路を `Sora::ProcessAudio` または `UnityAudioDevice::ProcessAudioData` に設ける。
- 有効 / 無効を Unity C# の `Sora.Config` から設定できるようにする (例: `EnableRNNoise`)。既定は無効とし、既存の挙動は変えない。
- RNNoise の組み込み方法 (DEPS に追加 / ソース同梱 / パッケージ済みバイナリ) と、モデルファイルの同梱・取得方法を決める。Windows / macOS / Android / iOS の各ターゲットでビルドできるようにする。
- libwebrtc のソフトウェア NS と RNNoise を同時に有効にしたときの二重適用の扱い (併用か排他か) を決める。
- 現状の入力経路は 48kHz ステレオのみで、リサンプリングは行っていない。48kHz 以外の入力を扱うかどうかを定める。

## 完了条件

- Unity C# から RNNoise の有効 / 無効を設定できる
- `Sora.Config.UnityAudioInput = true` の経路で RNNoise が送信音声に適用される
- RNNoise 無効時は既存の挙動 (libwebrtc 既定の NS) を変えない
- Windows / macOS / Android / iOS の各ビルドが通る
- `CHANGES.md` の `## develop` に `[ADD]` エントリを追記する

## pending にした理由

2026-09-10 に pending にする。

- RNNoise は外部のネイティブライブラリであり、`DEPS` への追加、各プラットフォーム向けのビルド、モデルファイルの配布・取得方法など、依存の組み込み方に設計判断が必要である。
- 適用位置 (Unity 入力経路のどこで掛けるか)、libwebrtc のソフトウェア NS との併用・排他、Unity C# 側の API の形が未確定である。
- これらが決まるまで実装方針を確定できないため保留とする。対応を再開するときは reopened にしてから、依存の組み込み方法と API を確定する。
