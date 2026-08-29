# SoraWrapper を受け取る C ABI 関数に null チェックを追加する

- Priority: Critical
- Created: 2026-08-27
- Branch: feature/fix-c-abi-null-check
- Polished: 2026-08-29
- Milestone: 2026.2.0

## 目的

`src/unity.cpp` の C ABI エントリポイント (`sora_set_on_rpc`, `sora_disconnect`, `sora_dispatch_events`, `sora_destroy` を含む `SoraWrapper*` を受け取る 35 関数) が `SoraWrapper*` の null チェックを一切行っておらず、`sora_create()` が `nullptr` を返した直後に呼び出されるだけで SEGV する。Unity Editor 再生直後や、`UnityContext` の初期化失敗時に踏みやすい経路のため、正式リリース前に必ず対応する。

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

`sora_set_on_rpc` を含む `src/unity.cpp` の `SoraWrapper*` を受け取る全 C ABI 関数は次のように `SoraWrapper*` にキャストして `sora` メンバを触るだけで、null チェックが存在しない:

```cpp
void sora_set_on_rpc(void* p, ...) {
  auto wsora = (SoraWrapper*)p;
  wsora->sora->SetOnRpc(...);
}
```

このため `p == nullptr` の状態で `wsora->sora` を触った瞬間に SEGV する。同様のパターンが `SoraWrapper*` を受け取る 35 関数すべてに存在する (`sora_destroy` を除く 34 関数は `wsora->sora` を逆参照し、`sora_destroy` は `delete` のみ)。

対象は `SoraWrapper*` を受け取る関数のみで、`webrtc::MediaStreamTrackInterface*` や `AudioTrackSinkImpl*` など他の型を受け取る関数は対象外とする。

`UnityContext::IsInitialized() == false` になる主な経路:
- Unity Editor 起動直後で `UnityPluginLoad` が呼ばれる前
- graphics device の初期化が失敗した場合
- 別 issue で扱う `UnityContext::Init` のログ初期化 silent-fail 経路

## 設計方針

- `src/unity.cpp` の `SoraWrapper*` を受け取る全 C ABI 関数の入り口に `SoraWrapper*` の null チェックを追加する
- チェックの重複を避けるため、以下のようなマクロで一括包装するのが妥当。既存の `auto wsora = (SoraWrapper*)p;` 宣言はマクロに置き換える (同じスコープに `wsora` を二重宣言できないため)

```cpp
#define SORA_ABI_GUARD(p) \
  auto wsora = static_cast<SoraWrapper*>(p); \
  if (wsora == nullptr || wsora->sora == nullptr) return
```

- 戻り値がある関数用に `SORA_ABI_GUARD_RET(p, ret)` のような版も用意する
- 引数レベルの検証 (`sora_send_message` の `label`、`sora_process_audio` の `buf` / `offset` / `samples` など) は本 issue の対象外とし、各引数に特化した別 issue で対応する
- C# 側 `Sora()` コンストラクタでは `p == IntPtr.Zero` を検出して `InvalidOperationException` を投げる
  - チェックは `GCHandle.Alloc(this)` より前に行い、例外でオブジェクトが破棄された場合の GCHandle リークを避ける
  - 例外メッセージには `UnityContext::IsInitialized() == false` の可能性がある旨を明示する

## 完了条件

- `src/unity.cpp` の `SoraWrapper*` を受け取る全 C ABI エントリで null チェックが行われている
- C# 側 `Sora()` コンストラクタで `sora_create()` が `IntPtr.Zero` を返した場合の防御が入っている
- Unity Editor で `UnityContext` 未初期化状態から `new Sora()` を実行しても SEGV しないことを確認する
- `CHANGES.md` の `## develop` に `[FIX] C ABI エントリの null チェック欠落による SEGV を修正する` を追記する
