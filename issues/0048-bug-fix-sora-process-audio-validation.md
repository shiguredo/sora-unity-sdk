# sora_process_audio に引数バリデーションを追加する

- Priority: Medium
- Created: 2026-08-27
- Branch: feature/fix-sora-process-audio-validation
- Polished: 2026-09-10

## 目的

`src/unity.cpp` の `sora_process_audio` が入力バリデーションをせず、負の offset や巨大値でバッファ外読み取り経由の SEGV に至る経路を防ぐ。C ABI 境界での防御を強化する。

## 現状

`sora_process_audio(void* p, const void* buf, int offset, int samples)` は、音声データの `buf` と `offset` / `samples` をそのまま `Sora::ProcessAudio` へ渡す（`p` は `SoraWrapper*` であり音声データではない）。

- `src/sora.cpp` の `Sora::ProcessAudio` は `unity_adm_->ProcessAudioData((const float*)buf + offset, samples * 2)` を実行する

問題点:

- `offset < 0` の場合、ポインタ演算がバッファの先頭より前を指す
- `offset` が正の巨大値の場合も、ポインタ演算がバッファ外を指し、`ProcessAudioData` の読み取りで SEGV し得る
- `samples < 0` の場合、`samples * 2` が負値となり `ProcessAudioData` の `size` として不正に渡る
- `samples` が `INT_MAX / 2` を超える場合、`samples * 2` が signed overflow する
- `buf == nullptr` の場合、ポインタ演算と読み取りが未定義動作になる
- C ABI として C# / Unity 側の実装ミスで壊れた引数が渡ると即クラッシュする

なお、`SoraWrapper*`（`p`）の null チェックは 0012「SoraWrapper を受け取る C ABI 関数に null チェックを追加する」の範囲であり、本 issue では扱わない。0012 は全 C ABI 関数の入り口に一括ガードを導入する予定である。

また、0015「`Sora::ProcessAudio` の unity_adm_ UAF を修正する」は同関数の C ABI 形状（ID ベースのルックアップ等）を変更する計画がある。形状が変更された場合は、本 issue の検証も 0015 の変更後の形状に合わせて適用する（検証対象の `buf` / `offset` / `samples` は変わらない）。

また、バッファ長は C++ 側からは分からない。バッファ長 (`data.Length`) が既知なのは呼び出し元の C# 側 `SoraUnitySdkExamples/Assets/SoraUnitySdk/Sora.cs` の `Sora.ProcessAudio` だけであり、正の巨大な `offset` / `samples` によるバッファ外アクセスの完全な防御はそちらで行う。

## 設計方針

- `src/unity.cpp` の `sora_process_audio` の先頭で以下をチェックし、範囲外なら何もせず return する
  - `buf == nullptr`
  - `offset < 0`
  - `samples < 0`
  - `samples > INT_MAX / 2`（`samples * 2` の signed overflow 防止）
- `Sora.cs` の `Sora.ProcessAudio` でも `data.Length` を使った検証を行い、範囲外なら何もせず return する
  - `data == null` または `offset < 0` または `samples < 0` または `(long)offset + (long)samples * 2 > data.Length`
  - ネイティブ側は `(const float*)buf + offset` から `samples * 2` 個読み込むため、`offset + samples * 2 <= data.Length` の成立が必要（`offset` は float 配列のインデックス）
- 過剰な defensive 分岐を避けつつ、境界の入力検証は行う

## 完了条件

- `buf == nullptr`、負の `offset` / `samples`、`samples * 2` が overflow する値で `sora_process_audio` を呼んでも SEGV しない（`p` の null は 0012 の範囲）
- バッファ長を超える `offset` / `samples`（巨大値）で `Sora.ProcessAudio` を呼んでも SEGV しない
- 正常値は現状通り動作する
- `CHANGES.md` の `## develop` に `[FIX] sora_process_audio に引数バリデーションを追加する` を追記する
