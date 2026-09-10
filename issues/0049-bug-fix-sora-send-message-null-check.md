# sora_send_message に引数バリデーションを追加する

- Priority: Medium
- Created: 2026-08-27
- Branch: feature/fix-sora-send-message-null-check
- Polished: 2026-09-10

## 目的

`src/unity.cpp` の `sora_send_message` は `label` を `std::string` に暗黙変換しているため、`label` が nullptr の場合に SEGV する。C# 側 `SoraUnitySdkExamples/Assets/SoraUnitySdk/Sora.cs` の `Sora.SendMessage` は `string label` をそのまま C ABI へ渡すため、null の label はネイティブ側へ到達し得る（`#nullable enable` の下で `string label` は non-nullable だが、null を渡すとコンパイル警告になるだけで実行時には弾かれない）。C ABI 境界で明示的な引数検証を入れる。

## 現状

`sora_send_message` は以下のコードを含む。

- `wsora->sora->SendMessage(label, std::string(s, s + size))`

問題点:

- `label` は `const char*` として渡され、`Sora::SendMessage(const std::string&, const std::string&)` の引数への暗黙変換で `strlen(label)` が呼ばれる
- `label == nullptr` の場合、`strlen(nullptr)` は未定義動作になり SEGV する
- C# 側の P/Invoke は null 文字列を NULL ポインタでマーシャルする（空文字列だけが空文字列へのポインタになる）。`Sora.SendMessage`（公開 API）は label を検証せずそのまま渡すため、`label == nullptr` の経路は実際に存在する（`SendRpcMessage` はラベルが固定 `"rpc"` のため null にはならない）
- `buf == nullptr` の場合、`std::string(s, s + size)` の範囲 `[nullptr, nullptr + size)` は不正となり未定義動作になる
- `size < 0` の場合、`s + size` がバッファ先頭より前を指し、`std::string(s, s + size)` の範囲が不正になる
- なお、`Sora::SendMessage`（`src/sora.cpp`）は `signaling_ == nullptr` をガードしているが、`label` の `std::string` 変換は C ABI 関数から `Sora::SendMessage` を呼び出す際の引数構築時に行われるため、このガードでは防げない

## 設計方針

- `src/unity.cpp` の `sora_send_message` の先頭（`Sora::SendMessage` 呼び出し前）で以下をチェックし、無効なら何もせず return する
  - `label == nullptr`
  - `buf == nullptr`（`s` は `(const char*)buf`。`std::string(s, s + size)` の範囲構築が未定義動作になるため）
  - `size < 0`（`s + size` がバッファ先頭より前を指すため）
- `size == 0` は `std::string(s, s)` で空文字列になり、そのまま送信する（現状の動作を維持する）
- 無効な入力で何もせず return する方針は、0048「sora_process_audio に引数バリデーションを追加する」と同じとする
- `SoraWrapper*`（`p`）の null チェックは 0012「SoraWrapper を受け取る C ABI 関数に null チェックを追加する」の範囲であり、本 issue では扱わない。0012 は全 C ABI 関数の入り口に一括ガードを導入する予定である

## 完了条件

- `sora_send_message` の先頭で `label` / `buf` / `size` の引数検証が行われている
- `label = nullptr` で呼んでも SEGV しない
- `buf = nullptr` または `size < 0` で呼んでも SEGV しない
- 正常値（`size == 0` を含む）は現状通り動作する
- `CHANGES.md` の `## develop` に `[FIX] sora_send_message に引数バリデーションを追加する` を追記する
