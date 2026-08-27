# sora_send_message の label に null チェックを追加する

- Priority: Medium
- Created: 2026-08-27
- Branch: fix/sora-send-message-null-check
- Polished: {YYYY-MM-DD}

## 目的

`src/unity.cpp` の `sora_send_message` は `label` を `std::string` に暗黙変換しているため、`label` が nullptr の場合に SEGV する。C ABI 境界で明示的な null チェックを入れる。

## 現状

`sora_send_message` は以下のコードを含む。

- `wsora->sora->SendMessage(label, std::string(s, s + size))`

問題点:

- `label` は `const char*` として渡され、`std::string` への暗黙変換で `strlen(label)` が呼ばれる
- `label == nullptr` の場合、`strlen(nullptr)` は未定義動作
- C# 側の Marshal は通常 null 文字列を空文字にするが、明示のガードが無い
- C ABI 境界の防御としては不十分

## 設計方針

- 関数先頭で `if (label == nullptr) return;` を追加する
- 同時に `s == nullptr` や `size <= 0` のガードも合わせて確認する
- C ABI 全体の null ガード方針と揃える

## 完了条件

- `sora_send_message` に null チェックが入っている
- `label = nullptr` で呼んでも SEGV しない
- 正常値は現状通り動作する
- `CHANGES.md` の `## develop` に `[FIX]` を追記する
