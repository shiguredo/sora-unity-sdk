# H.265 映像コーデックパラメーターに対応する

- Priority: Medium
- Created: 2026-06-08
- Model: Composer 2.5
- Branch: feature/add-h265-video-codec-params
- Polished: 2026-06-08

## 目的

Sora Unity SDK に H.265 映像コーデックパラメーター指定を追加する。

`VideoCodecType.H265` と VP9 / AV1 / H.264 用の `Video*Params` は既にあるが、H.265 だけ未対応である。H.264 等と同じパターンで Sora Unity SDK 内を揃える。

## 優先度根拠

- `VideoVp9Params` / `VideoAv1Params` / `VideoH264Params` があるのに `VideoH265Params` だけない
- H.265 コーデック自体は Unity SDK で既に利用可能
- 接続時パラメーターを指定したい利用者向けに、Unity SDK 側の不足を解消する

## 現状

`Sora.Config` には `VideoVp9Params` / `VideoAv1Params` / `VideoH264Params` があるが、`VideoH265Params` はない。

`proto/sora_conf_internal.proto` の `ConnectConfig` には `video_vp9_params = 250` / `video_av1_params = 251` / `video_h264_params = 252` があるが、`video_h265_params` はない。

`Sora.cs` の `Connect()` では `cc.video_h264_params = config.VideoH264Params` まで設定しているが、H.265 相当の代入はない。

`src/sora.cpp` では VP9 / AV1 / H.264 について JSON 文字列を parse して `SoraSignalingConfig` へ渡しているが、`video_h265_params` 相当の処理はない。

## 設計方針

- 既存 3 コーデックと同じ JSON 文字列プロパティとして `Sora.Config.VideoH265Params` を追加する
- 未指定時は `""` のまま。`src/sora.cpp` 側で `.empty()` なら parse しない（既存と同じ）
- 無効 JSON の場合は `RTC_LOG(LS_WARNING)` を出して設定をスキップし、connect 自体は続行する（既存と同じ）
- H.265 の JSON キーは H.264 と異なる。例: H.264 は `{"profile_level_id":"42e01f"}`、H.265 は `{"level_id":93}` 等
- `b_frame` は対応しない
- 変更対象:
  - `proto/sora_conf_internal.proto` に `string video_h265_params = 253;` を追加
  - `src/sora.cpp` に H.264 と同型の parse / 代入を追加
  - `SoraUnitySdkExamples/Assets/SoraUnitySdk/Sora.cs` に `VideoH265Params` と `Connect()` マッピングを追加
  - `python3 run.py build <target>` で Generated / native ヘッダを再生成
  - `CHANGES.md` に `[ADD]` を追記
- Samples への追加は任意

## 完了条件

- `Sora.Config.VideoH265Params` から `src/sora.cpp` 経由で `SoraSignalingConfig::video_h265_params` へ値が渡る
- 未指定（空文字）時は既存コーデックパラメーターと同様、Sora 側デフォルトが使われる
- 後方互換を壊さない（新規プロパティ追加のみ）
- `CHANGES.md` の `## develop` に `[ADD] Sora.Config.VideoH265Params を追加する` を追記する

## 解決方法

1. `proto/sora_conf_internal.proto` に `video_h265_params = 253` を追加する
2. `python3 run.py build <target>` で Generated / native ヘッダを再生成する
3. `Sora.cs` に `VideoH265Params` と `Connect()` マッピングを追加する
4. `src/sora.cpp` に H.264 と同型の `video_h265_params` parse ブロックを追加する
5. `VideoCodecType = H265` と検証用 JSON を設定して接続し、接続設定 JSON に `video_h265_params` が含まれ、Sora への connect で反映されることを確認する
6. `CHANGES.md` に `[ADD]` を追記する
