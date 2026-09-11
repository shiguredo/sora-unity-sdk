# SoraSample.cs の未使用 using とコメントアウトサンプルを整理する

- Priority: Low
- Created: 2026-08-27
- Branch: fmt/clean-sora-sample-usings
- Polished: 2026-09-11

## 目的

`SoraUnitySdkExamples/Assets/SoraSample.cs` に残っている未使用 using 宣言と `OnCapturerFrame` のサンプルコメントアウトを整理する。

## 現状

`SoraSample.cs` の using 宣言のうち以下は使われていない。

- `using System.Runtime.InteropServices;` — `Marshal.Copy` などはコメントアウトブロック内でしか登場しない
- `using Unity.Collections.LowLevel.Unsafe;` — `UnsafeUtility` は参照されておらず、`NativeArray<float>` は完全修飾で書かれている
- `using System.IO;` — `System.IO.File.Exists` / `System.IO.File.ReadAllText` はいずれも完全修飾で呼ばれている

`OnCapturerFrame` のサンプルコードは 18 行にわたってコメントアウトされたまま残っている（コメント行 15 行と空行 3 行）。実行されないサンプルコードを掲載し続けているのは「Don't live with broken windows」に反する。

## 設計方針

- 上記の未使用 using を削除する
- `OnCapturerFrame` のサンプルは以下のいずれかで整理する
  - 現行 API の `SoraConf.VideoFrame` に合わせた動く形のミニマルサンプルに書き直す
  - 書き直さない場合はコメントアウト塊を丸ごと削除する
- 挙動変更は無い（未使用 using とコメントアウトの掃除）

## 完了条件

- `SoraSample.cs` の using 宣言から未使用の 3 件が消えている
- `OnCapturerFrame` 周辺のサンプルコメントアウトが整理されている（動くミニマルサンプルか、コメントアウト塊の削除のみ）
- サンプルシーンでの動作に回帰が無い
