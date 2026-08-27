# run.py の _format パターンに .mm と .cc を追加する

- Priority: Medium
- Created: 2026-08-27
- Branch: fix/run-py-format-patterns
- Polished: {YYYY-MM-DD}

## 目的

`run.py` の `_format` 実装が `src/**/*.h` と `src/**/*.cpp` しか clang-format 対象にしていないため、`.mm` と `.cc` の 3 ファイルが恒久的に整形対象から抜けている問題を解消する。

## 現状

`run.py` の `_format` 内のパターン列挙には `src/**/*.h` と `src/**/*.cpp` のみが並んでいる。

これにより次の 3 ファイルは `python3 run.py format` を実行しても永久に clang-format の対象にならない。

- `src/unity_camera_capturer_metal.mm`
- `src/mac_helper/ios_audio_init.mm`
- `src/android_helper/jni_onload.cc`

`.clang-format` には ObjC 用のセクションが定義されているのに、対象ファイルが除外されているため活かせていない。フォーマット規約の徹底が形骸化している。

## 設計方針

- `run.py` の `_format` パターンに `src/**/*.mm` と `src/**/*.cc` を追加する
- 追加した後 `python3 run.py format` を実行し、3 ファイルに整形の差分が発生するかを確認する
- 差分がある場合は同 PR 内で整形結果を含める
- `.clang-format` の ObjC 設定が意図通り効いているかも合わせて確認する

## 完了条件

- `run.py _format` のパターンに `.mm` と `.cc` が含まれている
- 3 ファイルが `python3 run.py format` の対象になっている
- 整形適用済みで差分の無い状態になっている
- `CHANGES.md` の `## develop` に `[FIX] run.py の _format パターンに .mm と .cc を追加する` を追記する
