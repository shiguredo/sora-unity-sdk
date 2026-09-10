# IdPointer::Register の counter_ が 0 に戻る場合の防御を入れる

- Priority: Medium
- Created: 2026-08-27
- Branch: feature/fix-id-pointer-counter-overflow
- Polished: 2026-09-10

## 目的

`src/id_pointer.cpp` の `IdPointer::Register` が発行する ID が、`counter_` のオーバーフローによって 0（無効 ID のセンチネル値）と衝突する経路を塞ぐ。

## 現状

`src/id_pointer.h` の `IdPointer` は `ptrid_t counter_ = 1` から始まり、`Register` で `map_[counter_] = p; return counter_++` する。

一方で、`UnityRenderer::GetVideoSinkId` や `sora_get_video_sink_id_from_video_track` などの API は「見つからない」を 0 で返しており、`ptrid_t` の 0 は「無効 ID」のセンチネル値として扱われている。

`counter_` は `unsigned` 相当なので `UINT_MAX` の次に 0 に戻る。実運用で 40 億回の `Register` を単一プロセスで踏むのは現実的ではないが、コード上は防御が無い状態。

## 設計方針

- `Register` は次の 1 本のループで使用可能な ID を探し、登録後に `counter_++` する
  - `counter_ == 0`（無効 ID のセンチネル値）または `map_` に既に存在する ID である間、`counter_++` を繰り返す
  - `counter_` が `UINT_MAX` から 0 に戻った周回後も、0 を飛ばし、まだ `map_` に残っている生存 ID を再利用しない
- `map_` の ID が全て埋まるにはメモリ上で 2^32 個の生存オブジェクトが必要で非現実的なため、ループが無限に続くケースは考慮しない
- 単一 `mutex_` で `Register` は既にシリアライズされているため、追加のロック機構は不要
- `IdPointer` は 0011 と 0014 で std::shared_ptr / std::weak_ptr の弱参照ベースの API 形状へ再設計される予定であり、本 issue の変更は `IdPointer::Register` の実装内に留め、再設計後はその形状（テンプレート化 `IdPointer<T>` 等）に合わせて防御を引き継ぐ

## 完了条件

- `Register` が返す ID が 0 になることは無い
- `counter_` が周回した場合も既存の生存 ID と衝突しない
- 既存の `Lookup` / `Unregister` の挙動に回帰が無い
- `CHANGES.md` の `## develop` に `[FIX] IdPointer::Register の counter_ 周回時に 0 と衝突する経路を塞ぐ` を追記する
