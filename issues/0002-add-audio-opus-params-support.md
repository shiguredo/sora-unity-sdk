# audio_opus_params に対応する

- Priority: Medium
- Created: 2026-06-08
- Model: Composer 2.5
- Branch: feature/add-audio-opus-params-support
- Polished: 2026-06-08

## 目的

Sora Unity SDK に `audio_opus_params` 指定を追加する。

`AudioCodecType` / `AudioBitRate` 等はあるが、Opus 固有パラメーター（例: `usedtx`, `maxplaybackrate`, `stereo`）を接続時に渡す項目がない。映像コーデックパラメーターと同様に、Sora Unity SDK 側で対応する。

## 優先度根拠

- 映像側は VP9 / AV1 / H.264 / H.265 の params 露出を揃えているが、音声 Opus params だけ未対応
- `AudioCodecType` / `AudioBitRate` では Opus 固有パラメーターを代替できない
- 接続時に Opus パラメーターを指定したい利用者向けに、Unity SDK 側の不足を解消する

## 現状

`Sora.Config` には `AudioCodecType` / `AudioBitRate` / `AudioStreamingLanguageCode` / `AudioSpeakerVolume` / `AudioMicrophoneVolume` があるが、`AudioOpusParams` に相当するプロパティはない。

`proto/sora_conf_internal.proto` の `ConnectConfig` にも `audio_opus_params` フィールドはない。

`src/sora.cpp` では `audio_codec_type` / `audio_bit_rate` は `SoraSignalingConfig` へ渡しているが、`audio_opus_params` 相当の処理はない。

## 設計方針

- 映像コーデックパラメーターと同じ JSON 文字列プロパティとして `Sora.Config.AudioOpusParams` を追加する
- 未指定時は `""` のまま。`src/sora.cpp` 側で `.empty()` なら parse しない（既存と同じ）
- 無効 JSON の場合は `RTC_LOG(LS_WARNING)` を出して設定をスキップし、connect 自体は続行する（既存と同じ）
- JSON 例: `{"usedtx":true}`, `{"maxplaybackrate":48000}`。詳細は Sora の Opus パラメーター仕様に従う
- 変更対象:
  - `proto/sora_conf_internal.proto` に `string audio_opus_params = 254;` を追加（0001 実装後の次番号。0001 未実装なら番号衝突しないか確認する）
  - `src/sora.cpp` に映像 params と同型の parse / 代入を追加
  - `SoraUnitySdkExamples/Assets/SoraUnitySdk/Sora.cs` に `AudioOpusParams` と `Connect()` マッピングを追加
  - `python3 run.py build <target>` で Generated / native ヘッダを再生成
  - `CHANGES.md` に `[ADD]` を追記
- Samples への追加は任意

## 完了条件

- `Sora.Config.AudioOpusParams` から `src/sora.cpp` 経由で `SoraSignalingConfig::audio_opus_params` へ値が渡る
- 未指定（空文字）時は既存設定項目と同様、Sora 側デフォルトが使われる
- 後方互換を壊さない（新規プロパティ追加のみ）
- `CHANGES.md` の `## develop` に `[ADD] Sora.Config.AudioOpusParams を追加する` を追記する

## 解決方法

1. `proto/sora_conf_internal.proto` に `audio_opus_params` を追加する
2. `python3 run.py build <target>` で Generated / native ヘッダを再生成する
3. `Sora.cs` に `AudioOpusParams` と `Connect()` マッピングを追加する
4. `src/sora.cpp` に映像 params と同型の `audio_opus_params` parse ブロックを追加する
5. 検証用 JSON を設定して接続し、接続設定 JSON に `audio_opus_params` が含まれ、Sora への connect で反映されることを確認する
6. `CHANGES.md` に `[ADD]` を追記する
