# Sora::RenderCallbackStatic と ~Sora の race による UAF を修正する

- Priority: Critical
- Created: 2026-08-27
- Branch: fix/render-callback-idpointer-race
- Polished: {YYYY-MM-DD}
- Milestone: 2026.2.0

## 目的

Unity のレンダースレッドから呼ばれる `Sora::RenderCallbackStatic` が、`IdPointer::Lookup` で `Sora*` を得た直後に `~Sora` が完走すると use-after-free を引き起こす。Sora を破棄する `Sora.Dispose()` を実行するタイミングと Unity レンダースレッドのフレーム描画が競合するため、通常運用でも踏みうる致命的な UAF。

## 現状

`src/sora.cpp` の `Sora::RenderCallbackStatic` は次のように `IdPointer::Lookup` で自身を検索する:

```cpp
void UNITY_INTERFACE_API Sora::RenderCallbackStatic(int event_id) {
  auto sora = static_cast<Sora*>(IdPointer::Instance().Lookup(event_id));
  if (sora == nullptr) {
    return;
  }
  sora->RenderCallback();
}
```

`~Sora` の先頭では次のように `IdPointer::Unregister` を呼んで自身をマップから外す:

```cpp
Sora::~Sora() {
  IdPointer::Instance().Unregister(ptrid_);
  renderer_.reset();
  ...
}
```

`IdPointer::Lookup` と `IdPointer::Unregister` は同じ `mutex_` で保護されているため、Lookup 中に Unregister が完了することはない。しかし Lookup が return した直後 `sora->RenderCallback()` を実行する前に、Unity のメインスレッドで `SoraWrapper` を `delete` し `Sora` の shared_ptr の最後の参照が落ちて `~Sora` が完走してしまうと、Lookup で得た `Sora*` は既に破棄済みメモリを指すことになる。

このケースでは `~Sora` は `unity_adm_ = nullptr;` や `capturer_ = nullptr;` を実行しているため、`RenderCallback` はこれらを触った瞬間に SEGV する。

`SoraWrapper` は `std::shared_ptr<sora_unity_sdk::Sora>` を保持しており、通常運用では SoraWrapper が唯一の owner になるため、`sora_destroy` (`delete wsora`) 直後にこの race window が開く。

## 設計方針

- `IdPointer` を weak_ptr ベースに変更する
  - `Sora` を `enable_shared_from_this` にする (既にそうなっている)
  - `IdPointer::Register` は `void*` ではなく `std::weak_ptr<T>` を保持する
  - `IdPointer::Lookup` は `weak_ptr::lock()` で `shared_ptr` を返し、呼び出し側が保持している間は Sora の寿命が延びる
- `RenderCallbackStatic` は Lookup で得た `shared_ptr<Sora>` をローカル変数に保持したまま `RenderCallback()` を呼び出す
  - Unity のメインスレッドが同時に `sora_destroy` を実行しても、この shared_ptr が残っている限り `~Sora` は動かない
- 同じ設計変更を `UnityRenderer::Sink::TextureUpdateCallback` にも適用する (別 issue で扱う Sink race と方針を揃える)
- `IdPointer` のテンプレート化 (`IdPointer<T>`) または個別ラッパーの導入は、既存の複数用途 (Sora, Sink) との互換性を考慮して設計する

## 完了条件

- `IdPointer::Lookup` が破棄済み `Sora` を返さないことを設計レベルで保証する (weak_ptr / shared_ptr)
- `Sora.Dispose()` と Unity レンダースレッドのフレーム描画が競合しても UAF が発生しないことを確認する
- Unity Editor で Play / Stop を繰り返しても Sora の破棄と RenderCallback の race で SEGV しないことを確認する
- `CHANGES.md` の `## develop` に `[FIX] Sora::RenderCallbackStatic と ~Sora の race による UAF を修正する` を追記する
