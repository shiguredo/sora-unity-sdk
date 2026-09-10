# SoraSample のオーディオレベル描画を最適化する

- Priority: Medium
- Created: 2026-08-27
- Branch: update/optimize-sample-audio-level-drawing
- Polished: 2026-09-10

## 目的

`SoraUnitySdkExamples/Assets/SoraSample.cs` のオーディオレベル描画を、毎フレーム行う全ピクセル塗りから、更新頻度を制限した描画に変える。サンプルはユーザーが真似する参照実装であるため、負荷の少ない書き方を提示する。

## 現状

`SoraSample.cs` の `UpdateAudioLevelTextures` は 240 × 20 = 4800 ピクセルのテクスチャに対して、まず背景色で全塗りしてから塗り上げる領域だけを上塗りする方式を毎フレーム全クライアント分実行する。`Update()` から毎フレーム呼び出されるため、4800 ピクセル × クライアント数分の CPU 書き込みと、`SetPixels32` + `Apply` によるテクスチャアップロードが毎フレーム発生する。

クライアント数分このループが走るため、参加者が増えるほど負荷が線形に増える。サンプルとしても「Update から毎フレーム全 sink をぶん回す」書き方は誤解を招きやすい。

## 設計方針

- 描画方式（`Texture2D` + `SetPixels32`）は変更せず、更新のタイミングだけを変える
  - `RawImage` の描画に `MaterialPropertyBlock` は効かないため、fill 幅を渡す方式は採用しない
  - `Graphics.Blit` を使うには fill 幅を制御するシェーダーの追加が必要になり、サンプルとしては過剰なため採用しない
- サンプル内でオーディオレベル描画の頻度制御（例えば 10 Hz 程度）を入れる。前回の更新時刻（例えば `Time.time`）を保持し、更新間隔未満なら `UpdateAudioLevelTextures` の処理自体をスキップする
- 前回の更新時から `AudioTrackSink` の `level` が変化していない場合は再描画しない
- `SoraUnitySdkExamples/README.md` で「本サンプルはあくまでデモであり、更新頻度は参照実装に応じて調整すること」を明記する

## 完了条件

- オーディオレベルテクスチャの更新が毎フレームから 10 Hz 程度に制限され、`Update()` の毎フレームでは全ピクセル塗り（`SetPixels32` + `Apply`）が実行されない
- 既存のオーディオレベルインジケータの見た目に回帰が無い
- `SoraUnitySdkExamples/README.md` に更新頻度の注意書きが追記されている
