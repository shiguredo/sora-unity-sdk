# UnityRenderer Sink の TextureUpdateCallback race による SEGV を修正する

- Priority: Critical
- Created: 2026-08-27
- Branch: fix/unity-renderer-sink-race
- Polished: 2026-08-29
- Milestone: 2026.2.0

## 目的

`UnityRenderer::Sink::TextureUpdateCallback` が Unity のレンダースレッドから呼ばれるが、同時にトラックの削除で `~Sink` が走ると use-after-free を引き起こし SEGV する既知のバグを修正する。ソース本体に `TODO(melpon)` として race の存在が明記されており、Unity 上でトラックの追加・削除を高頻度に繰り返すシナリオで踏む可能性がある。

## 現状

`src/unity_renderer.cpp` の `UnityRenderer::Sink::TextureUpdateCallback` は Unity のレンダースレッドから呼ばれ、以下の流れで実行される:

```cpp
Sink* p = (Sink*)IdPointer::Instance().Lookup(params->userData);
if (p == nullptr) {
  return;
}
// TODO(melpon): p を取得した直後、updating_ = true にするまでの間に Sink が削除されたら
// セグフォしてしまうので、問題になるようなら Lookup の時点でロックを獲得する必要がある

if (p->deleting_) {
  p->updating_ = false;
  return;
}
p->updating_ = true;
```

`~Sink` は次のように動作する:

```cpp
deleting_ = true;
while (updating_) {
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
}
```

`IdPointer::Lookup` は内部で `mutex_` を取っており、ここまでは安全に `Sink*` を返す。しかし Lookup が return した直後、Unity のメインスレッドで `RemoveTrack` などから `~Sink` が完走する race window が存在する。この window で `~Sink` が完走した後にレンダースレッドが `p->deleting_` / `p->updating_ = true` を触ると、既に破棄されたメモリへの UAF となる。

さらに `~Sink` は無限ループの busy-wait でスピンしており、`updating_` が false になるまで 10ms 単位で回すため、異常時に Unity メインスレッドをフリーズさせる副作用もある。

## 設計方針

- `IdPointer::Lookup` の設計を変更し、生ポインタではなく `std::shared_ptr<Sink>` を返す形にする
  - `Sink` を `enable_shared_from_this` にし、`IdPointer` は `weak_ptr` を保持する
  - `Lookup` は `weak_ptr::lock()` で `shared_ptr` を返し、呼び出し側が保持している間は Sink の寿命が延びる
  - これにより `TextureUpdateCallback` 内で Sink が破棄されない保証を得られる
- `weak_ptr::lock()` が成功するには Sink を `shared_ptr` で所有している必要があるため、`UnityRenderer::sinks_` の所有権を `std::unique_ptr` から `std::shared_ptr` に変更する
  - `AddTrack` / `RemoveTrack` / `ReplaceTrack` も shared_ptr ベースに追随させる
  - コンストラクタ内の `Register(this)` は `shared_from_this()` が使えないため、Sink を `shared_ptr` で生成してから `Register` する形に変更する
- `IdPointer` の変更は `UnityRenderer::Sink` だけでなく `Sora` の `RenderCallbackStatic` からの Lookup にも同じ問題があるため、両者で一貫した設計にする (別 issue で対応する `RenderCallback` race と方針を揃える)
  - API 形状はテンプレート化 `IdPointer<T>` または個別ラッパーをその別 issue と合わせて決め、`Sora` 側の Lookup 呼び出しの変更はその別 issue の範囲とする
- `~Sink` の busy-wait を `std::condition_variable` ベースに置き換える対応は、別 issue で対応する Sink デストラクタの busy-wait 置き換えに委ねる (本 issue の対象外)
- `deleting_` / `updating_` は既に `std::atomic<bool>` であり、shared_ptr 化により UAF そのものが消えるため、atomic 化の追加対応は不要

## 完了条件

- `IdPointer::Lookup` が破棄済み `Sink` を返さないことを設計レベルで保証する (shared_ptr / weak_ptr)
- `UnityRenderer::Sink::TextureUpdateCallback` の TODO コメントが解消されている
- Unity 上でトラック追加・削除を高頻度に繰り返すソークテストで SEGV が発生しないことを確認する
- `CHANGES.md` の `## develop` に `[FIX] UnityRenderer::Sink::TextureUpdateCallback の race による SEGV を修正する` を追記する
