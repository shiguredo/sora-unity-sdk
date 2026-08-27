# SoraSample のオーディオレベル描画を最適化する

- Priority: Medium
- Created: 2026-08-27
- Branch: update/optimize-sample-audio-level-drawing
- Polished: {YYYY-MM-DD}

## 目的

`SoraUnitySdkExamples/Assets/SoraSample.cs` のオーディオレベル描画を `Graphics.Blit` などに切り替え、毎フレーム全ピクセル塗りを避ける。サンプルはユーザーが真似する参照実装であるため、負荷の少ない書き方を提示する。

## 現状

`SoraSample.cs` の `UpdateAudioLevelTextures` は 240 × 20 = 4800 ピクセルのテクスチャに対して、まず背景色で全塗りしてから塗り上げる領域だけを上塗りする方式を毎フレーム全クライアント分実行する。

クライアント数分このループが走るため、参加者が増えるほど負荷が線形に増える。サンプルとしても「Update から毎フレーム全 sink をぶん回す」書き方は誤解を招きやすい。

## 設計方針

- 塗り替え対象を `RenderTexture` にして `Graphics.Blit` で書く、あるいは `MaterialPropertyBlock` で fill 幅を渡す方式にする
- サンプル内でオーディオレベル描画の頻度制御（例えば 10 Hz 程度）を入れる
- README / サンプル README で「本サンプルはあくまでデモであり、頻度と描画方式は参照実装に応じて調整すること」を明記する

## 完了条件

- クライアント数が増えても描画コストが線形に膨らまない実装になっている
- 既存のオーディオレベルインジケータの見た目に回帰が無い
- サンプルの説明に描画方式の注意書きが追記されている
