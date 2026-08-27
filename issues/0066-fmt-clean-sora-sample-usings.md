# SoraSample.cs の未使用 using とコメントアウトサンプルを整理する

- Priority: Low
- Created: 2026-08-27
- Branch: fmt/clean-sora-sample-usings
- Polished: {YYYY-MM-DD}

## 目的

`SoraUnitySdkExamples/Assets/SoraSample.cs` に残っている未使用 using 宣言と `OnCapturerFrame` のサンプルコメントアウトを整理する。

## 現状

`SoraSample.cs` の using 宣言のうち以下は使われていない。

- `using System.Runtime.InteropServices;` — `Marshal.Copy` などはコメントアウトブロック内でしか登場しない
- `using Unity.Collections.LowLevel.Unsafe;` — `UnsafeUtility` は参照されておらず、`NativeArray<float>` は完全修飾で書かれている
- `using System.IO;` — `System.IO.File.Exists` / `System.IO.File.ReadAllText` はいずれも完全修飾で呼ばれている

`OnCapturerFrame` のサンプルコードは 20 行にわたって全体がコメントアウトされたまま残っている。サンプルとして動かないコードを掲載し続けているのは「Don't live with broken windows」に反する。

## 設計方針

- 上記の未使用 using を削除する
- `OnCapturerFrame` のサンプルは以下のいずれかで整理する
  - 動く形のミニマルサンプルに書き直す
  - README のリンクへ誘導し、コメントアウト塊は削除する
- 挙動変更は無い（未使用 using とコメントアウトの掃除）

## 完了条件

- `SoraSample.cs` の using 宣言から未使用の 3 件が消えている
- `OnCapturerFrame` 周辺のサンプルコメントアウトが整理されている（動くサンプルか、リンク誘導のみ）
- サンプルシーンでの動作に回帰が無い
