# IdPointer::Register の counter_ が 0 に戻る場合の防御を入れる

- Priority: Medium
- Created: 2026-08-27
- Branch: fix/id-pointer-counter-overflow
- Polished: {YYYY-MM-DD}

## 目的

`src/id_pointer.cpp` の `IdPointer::Register` が発行する ID が、`counter_` のオーバーフローによって 0（無効 ID のセンチネル値）と衝突する経路を塞ぐ。

## 現状

`src/id_pointer.h` の `IdPointer` は `ptrid_t counter_ = 1` から始まり、`Register` で `map_[counter_] = p; return counter_++` する。

一方で、`UnityRenderer::GetVideoSinkId` や `sora_get_video_sink_id_from_video_track` などの API は「見つからない」を 0 で返しており、`ptrid_t` の 0 は「無効 ID」のセンチネル値として扱われている。

`counter_` は `unsigned` 相当なので `UINT_MAX` の次に 0 に戻る。実運用で 40 億回の `Register` を単一プロセスで踏むのは現実的ではないが、コード上は防御が無い状態。

## 設計方針

- `Register` 内で `counter_` が加算後に 0 になった場合はスキップして 1 から再開する
- あるいは `counter_` を加算する前に 0 と比較して、0 なら 1 に補正する
- 既存の `map_` に既に含まれている ID とも衝突しないよう、`while (map_.count(counter_) != 0) counter_++;` のような防御も併せて入れる
- 単一 mutex で `Register` は既にシリアライズされているため、追加のロック機構は不要

## 完了条件

- `Register` が返す ID が 0 になることは無い
- `counter_` が周回した場合も既存の生存 ID と衝突しない
- 既存の `Lookup` / `Unregister` の挙動に回帰が無い
- `CHANGES.md` の `## develop` に `[FIX] IdPointer::Register の counter_ 周回時に 0 と衝突する経路を塞ぐ` を追記する
