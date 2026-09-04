# src/converter.cpp の boost::json::parse を error_code 版に統一する

- Priority: Medium
- Created: 2026-08-27
- Branch: feature/fix-converter-boost-json-parse
- Polished: 2026-09-04

## 目的

`src/converter.cpp` 内で `boost::json::parse` を `error_code` 引数を取らないオーバーロード（例外送出型）で呼び出している箇所を、`error_code` 引数を取るオーバーロードに統一する。空文字列や JSON 構文エラーの文字列が渡された場合の例外送出による Unity プロセスのクラッシュを防ぐ。

## 現状

`src/converter.cpp` の `ConvertToVideoCodecCapability` 内で、`codec.parameters` と `engine.parameters` に対して `boost::json::parse` を `error_code` 引数を取らないオーバーロードで呼び出している。同じファイル内の `ConvertToVideoCodecPreference` も `codec.parameters` に対して同様の呼び出しになっている。

同じファイル内の `ConvertToForwardingFilter` では `boost::json::parse(filter.metadata, ec)` のように `error_code` を受け取り、`ec` が真なら `RTC_LOG(LS_WARNING)` を出して設定をスキップするパターンで書かれている。

`error_code` 引数を取らない `boost::json::parse` は JSON の構文エラー時に `boost::system::system_error` を送出する。呼び出し元の C ABI 関数には catch が無いため、例外が Unity プロセスまで伝播してクラッシュする。

- `ConvertToVideoCodecCapability` の呼び出し元: `src/unity.cpp` の `sora_video_codec_capability_to_json` / `sora_video_codec_capability_to_json_size` / `sora_create_video_codec_preference_from_implementation` / `sora_create_video_codec_preference_from_implementation_size`。いずれもユーザー入力の JSON を `jsonif::from_json` で解釈する
- `ConvertToVideoCodecPreference` の呼び出し元: `src/unity.cpp` の `sora_video_codec_preference_to_json` / `sora_video_codec_preference_to_json_size` / `sora_video_codec_preference_merge` / `sora_video_codec_preference_merge_size` / `sora_video_codec_preference_has_implementation`、および `src/sora.cpp` の `Sora::DoConnect`（connect 設定の `video_codec_preference`）

空文字列（`parameters` 未指定時のデフォルト値）や構文エラーの JSON 文字列を渡すと、`sora_video_codec_capability_to_json` 等で再現する。

## 設計方針

- `src/converter.cpp` の `boost::json::parse` を全て `error_code` 引数を取るオーバーロードに置き換える。`error_code` は `boost::system::error_code` を使い、`ConvertToForwardingFilter` の `if (ec)` / `else` パターンに表現を揃える
- `ec` が真なら `RTC_LOG(LS_WARNING)` を出し、当該 params はデフォルト値（未設定）のまま扱う。`boost::json::value_to` による変換は `ec` が偽のとき（`else` 側）のみ行う。error_code 版の `boost::json::parse` は失敗時に null の `value` を返すため、そのまま `value_to` に渡してはならない
- ユーザー入力に起因する JSON 構文エラーでは例外を送出しない
- `ConvertToForwardingFilter` の既存パターンと表現を揃える

## 完了条件

- `src/converter.cpp` 内の `boost::json::parse` 呼び出しが全て `error_code` 引数を取るオーバーロードになっている
- 空文字列や JSON 構文エラーを含む `codec.parameters` / `engine.parameters` を渡しても例外が送出されず、対象 params のみスキップされる
- `sora_video_codec_capability_to_json` / `sora_video_codec_preference_to_json` と `video_codec_preference` を含む正常系接続で、既存の動作が回帰していない
- `CHANGES.md` の `## develop` に `[FIX] src/converter.cpp の boost::json::parse を error_code 版に統一して例外送出を防ぐ` を追記する
