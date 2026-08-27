# C ABI 全関数に SoraWrapper の null チェックを追加する

- Priority: Critical
- Created: 2026-08-27
- Branch: fix/c-abi-null-check
- Polished: {YYYY-MM-DD}
- Milestone: 2026.2.0

## 目的

`src/unity.cpp` の C ABI エントリポイント (`sora_set_on_rpc`, `sora_disconnect`, `sora_dispatch_events`, `sora_destroy` を含む 40 以上の関数) が `SoraWrapper*` の null チェックを一切行っておらず、`sora_create()` が `nullptr` を返した直後に呼び出されるだけで SEGV する。Unity Editor 再生直後や、`UnityContext` の初期化失敗時に踏みやすい経路のため、正式リリース前に必ず対応する。

## 現状

`src/unity.cpp` の `sora_create()` は次のように `UnityContext::IsInitialized() == false` の場合に nullptr を返す:

```cpp
void* sora_create() {
  ...
  auto context = &sora_unity_sdk::UnityContext::Instance();
  if (!context->IsInitialized()) {
    return nullptr;
  }
  ...
}
```

一方で C# 側 `SoraUnitySdkExamples/Assets/SoraUnitySdk/Sora.cs` の `Sora()` コンストラクタは、`sora_create()` の戻り値を検査せずに続く API を呼び出す:

```csharp
public Sora()
{
    p = sora_create();
    selfHandle = GCHandle.Alloc(this);
    commandBuffer = new UnityEngine.Rendering.CommandBuffer();
    sora_set_on_rpc(p, RpcCallback, GCHandle.ToIntPtr(selfHandle));
}
```

`sora_set_on_rpc` を含む `src/unity.cpp` の全 C ABI 関数は次のように `SoraWrapper*` にキャストして `sora` メンバを触るだけで、null チェックが存在しない:

```cpp
void sora_set_on_rpc(void* p, ...) {
  auto wsora = (SoraWrapper*)p;
  wsora->sora->SetOnRpc(...);
}
```

このため `p == nullptr` の状態で `wsora->sora` を触った瞬間に SEGV する。同様のパターンが `sora.cpp` を経由して呼ばれる 40 以上の関数すべてに存在する。

`UnityContext::IsInitialized() == false` になる主な経路:
- Unity Editor 起動直後で `UnityPluginLoad` が呼ばれる前
- graphics device の初期化が失敗した場合
- 別 issue で扱う `UnityContext::Init` のログ初期化 silent-fail 経路

## 設計方針

- `src/unity.cpp` の全 C ABI 関数の入り口に `SoraWrapper*` の null チェックを追加する
- チェックの重複を避けるため、以下のようなマクロで一括包装するのが妥当

```cpp
#define SORA_ABI_GUARD(p) \
  auto wsora = static_cast<SoraWrapper*>(p); \
  if (wsora == nullptr || wsora->sora == nullptr) return
```

- 戻り値がある関数用に `SORA_ABI_GUARD_RET(p, ret)` のような版も用意する
- C# 側 `Sora()` コンストラクタでは `p == IntPtr.Zero` を検出して例外化する
  - どのエラー種別を投げるかは要検討 (`InvalidOperationException` 相当)
  - 例外メッセージには `UnityContext::IsInitialized() == false` の可能性がある旨を明示する

## 完了条件

- `src/unity.cpp` の全 C ABI エントリで `SoraWrapper*` の null チェックが行われている
- C# 側 `Sora()` コンストラクタで `sora_create()` が `IntPtr.Zero` を返した場合の防御が入っている
- Unity Editor で `UnityContext` 未初期化状態から `new Sora()` を実行しても SEGV しないことを確認する
- `CHANGES.md` の `## develop` に `[FIX] C ABI エントリの null チェック欠落による SEGV を修正する` を追記する
