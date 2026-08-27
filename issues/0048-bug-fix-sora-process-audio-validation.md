# sora_process_audio に引数バリデーションを追加する

- Priority: Medium
- Created: 2026-08-27
- Branch: fix/sora-process-audio-validation
- Polished: {YYYY-MM-DD}

## 目的

`src/unity.cpp` の `sora_process_audio` が入力バリデーションをせず、負の offset や巨大値でバッファ外読み取り経由の SEGV に至る経路を防ぐ。C ABI 境界での防御を強化する。

## 現状

`sora_process_audio` は Unity から渡された `p` / `offset` / `samples` を使って以下を実行する。

- `wsora->sora->ProcessAudio(p, offset, samples)`
- 実装は `unity_adm_->ProcessAudioData((const float*)p + offset, samples * 2)`

問題点:

- `offset < 0` や `offset` が巨大値の場合、ポインタ演算がバッファ外を指す
- `samples * 2` が signed overflow を起こす可能性がある
- C ABI として C# / Unity 側の実装ミスで壊れた引数が渡ると即クラッシュする

## 設計方針

- `sora_process_audio` の先頭で `offset >= 0` および `samples >= 0` をチェックする
- `p == nullptr` チェックを併せて入れる（C ABI 全体の null チェック方針と揃える）
- 範囲外の場合は何もせず return する
- 過剰な defensive 分岐を避けつつ、境界の入力検証は行う

## 完了条件

- 負の offset や巨大値、null ポインタで `sora_process_audio` を呼んでも SEGV しない
- 正常値は現状通り動作する
- `CHANGES.md` の `## develop` に `[FIX]` を追記する
