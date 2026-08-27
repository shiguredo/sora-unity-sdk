# src/converter.cpp の boost::json::parse を error_code 版に統一する

- Priority: Medium
- Created: 2026-08-27
- Branch: fix/converter-boost-json-parse
- Polished: {YYYY-MM-DD}

## 目的

`src/converter.cpp` 内で `boost::json::parse` を `error_code` 引数を取らないオーバーロードで呼び出している箇所を、`error_code` 引数を取るオーバーロードに統一する。空文字列や不正 JSON が渡された場合の例外送出によるプロセスクラッシュを防ぐ。

## 現状

`src/converter.cpp` の `ConvertToVideoCodecCapability` 内で、`codec.parameters` と `engine.parameters` に対して `boost::json::parse` を `error_code` 引数を取らないオーバーロードで呼び出している。

同じファイル内の `ConvertToForwardingFilter` では `boost::json::parse(filter.metadata, ec)` のように `error_code` を受け取り、`ec` が真なら `RTC_LOG(LS_WARNING)` を出して設定をスキップするパターンで書かれている。

`ConvertToVideoCodecCapability` は `sora_get_video_codec_capability` から呼ばれる。ユーザーが `sora_video_codec_capability_to_json` 等で不正入力を渡すと再現し、`boost::system::system_error` 例外が Unity プロセス全体まで伝播してクラッシュする。

`ConvertToVideoCodecPreference` の `codec.parameters` に対する `boost::json::parse` も同様に例外送出型の呼び出しになっている。

## 設計方針

- `src/converter.cpp` の `boost::json::parse` を全て `error_code` 引数を取るオーバーロードに置き換える
- `ec` が真なら `RTC_LOG(LS_WARNING)` を出し、当該 params は空 / デフォルト値のまま扱う
- ユーザー入力に起因する不正 JSON では例外を送出しない
- `ConvertToForwardingFilter` の既存パターンと表現を揃える

## 完了条件

- `src/converter.cpp` 内の `boost::json::parse` 呼び出しが全て `error_code` 引数を取るオーバーロードになっている
- 空文字列や不正 JSON を含む `codec.parameters` / `engine.parameters` を渡しても例外が送出されず、対象 params のみスキップされる
- 既存の正常系接続が回帰していない
- `CHANGES.md` の `## develop` に `[FIX] src/converter.cpp の boost::json::parse を error_code 版に統一して例外送出を防ぐ` を追記する
