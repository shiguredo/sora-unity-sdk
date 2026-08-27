# Sora::OnRemoveTrack の operator[] を find に置き換える

- Priority: Medium
- Created: 2026-08-27
- Branch: fmt/on-remove-track-find
- Polished: {YYYY-MM-DD}

## 目的

`src/sora.cpp` の `Sora::OnRemoveTrack` で `connection_ids_[track->id()]` によって map の値を読んでいる箇所を `find` に置き換え、read only の意図を明確にしつつ意図せぬデフォルト挿入を防ぐ。

## 現状

`Sora::OnRemoveTrack` は `connection_ids_[track->id()]` で `std::map` の値を読み取っている。`std::map::operator[]` はキーが存在しない場合デフォルト構築した空文字列を map に挿入する副作用がある。

その後 `connection_ids_.erase(track->id())` を呼んでいるので最終的な状態としては問題ないが、「読むだけのつもりでデフォルト挿入する」書き方は意図が読みにくい。

`on_remove_media_stream_track_` に空文字列を渡すのが正しい振る舞いなのかも、`operator[]` に依存した副作用として読める。

## 設計方針

- `std::map::find` を使い、見つからない場合と見つかった場合を明示的に分岐する
- 見つからない場合の callback 呼び出しにどの connection_id を渡すべきかを明確化する（現状の挙動を維持するなら空文字列）
- `erase` は既存のまま残す

## 完了条件

- `Sora::OnRemoveTrack` から `connection_ids_[...]` の read が消えている
- `find` を使った明示的な分岐に置き換わっている
- 既存の `on_remove_media_stream_track_` コールバックの引数挙動に回帰が無い
