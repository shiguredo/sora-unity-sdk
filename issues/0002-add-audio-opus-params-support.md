# audio_opus_params に対応する

- Priority: Medium
- Created: 2026-06-08
- Model: Composer 2.5
- Branch: feature/add-audio-opus-params-support
- Polished: 2026-09-10

## 目的

Sora Unity SDK に `audio_opus_params` 指定を追加する。

`AudioCodecType` / `AudioBitRate` 等はあるが、Opus 固有パラメーター（例: `usedtx`, `maxplaybackrate`, `stereo`）を接続時に渡す項目がない。映像コーデックパラメーターと同様に、Sora Unity SDK 側で対応する。

## 優先度根拠

- 映像側は VP9 / AV1 / H.264 の params 露出を揃えているが、音声 Opus params だけ未対応（H.265 params は別途対応予定）
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
- `AudioOpusParams` は `AudioCodecType.OPUS` と併せて指定する必要がある。Sora サーバーは connect メッセージの `audio` オブジェクト内 `opus_params` を、`codec_type` が `OPUS`（または `MULTIOPUS`）の場合のみ受け付ける（サーバー側 `sora_media_audio:validate_audio/1`）。そのため `AudioCodecType` 未指定のまま `AudioOpusParams` だけを設定すると、Sora サーバー側で connect がエラーになる。映像側の `video_*_params` と同様に `codec_type` の指定が前提である
- `opus_params` の JSON キーと値は Sora サーバー側 `sora_media_audio_opus:validate_param/2` で検証される（例: `usedtx`, `stereo`, `maxplaybackrate`, `useinbandfec`, `channels`, `ptime`, `minptime`, `sprop_stereo`, `channel_mapping`, `num_streams`, `coupled_streams`）。SDK 側は JSON をそのまま渡すため特別な扱いはしない（既存と同じ）
- 変更対象:
  - `proto/sora_conf_internal.proto` に `string audio_opus_params = 254;` を追加（253 は `video_h265_params` 用に別途予約済みのため、実装順に関わらず衝突しない）
  - `src/sora.cpp` に映像 params と同型の parse / 代入を追加
  - `SoraUnitySdkExamples/Assets/SoraUnitySdk/Sora.cs` に `AudioOpusParams` と `Connect()` マッピングを追加
  - `python3 run.py build <target>` で Generated / native ヘッダを再生成
  - `CHANGES.md` に `[ADD]` を追記
- Samples への追加は任意

## 完了条件

- `Sora.Config.AudioOpusParams` から `src/sora.cpp` 経由で `SoraSignalingConfig::audio_opus_params` へ値が渡る
- 未指定（空文字）時は `opus_params` 自体が送信されず、既存設定項目と同様に Sora 側デフォルトが使われる（この場合 `AudioCodecType` の指定は不要）
- 後方互換を壊さない（新規プロパティ追加のみ）
- `CHANGES.md` の `## develop` に `[ADD] Sora.Config.AudioOpusParams を追加する` を追記する

## 解決方法

1. `proto/sora_conf_internal.proto` に `audio_opus_params` を追加する
2. `python3 run.py build <target>` で Generated / native ヘッダを再生成する
3. `Sora.cs` に `AudioOpusParams` と `Connect()` マッピングを追加する
4. `src/sora.cpp` に映像 params と同型の `audio_opus_params` parse ブロックを追加する
5. `AudioCodecType.OPUS` と検証用 JSON（例: `{"usedtx":true}`）を設定して接続し、connect メッセージの `audio` オブジェクト内 `opus_params` に反映されることを確認する（`SoraSignalingConfig::audio_opus_params` は Sora C++ SDK が `audio.opus_params` として送信する。`AudioCodecType` 未指定では Sora サーバーがエラーにするため、必ず `OPUS` を指定すること）
6. `CHANGES.md` に `[ADD]` を追記する
