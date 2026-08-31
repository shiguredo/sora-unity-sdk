# CommandBuffer と RenderTexture の Release/Destroy 漏れを修正する

- Priority: High
- Created: 2026-08-27
- Branch: fix/command-buffer-render-texture-leak
- Polished: 2026-08-31
- Milestone: 2026.2.0

## 目的

`Sora.cs` は `CommandBuffer` と `RenderTexture` を new するが、それぞれ Release/Destroy を明示的に呼ばず GC 依存になっている。Unity の `CommandBuffer.Release()` / `Object.Destroy(RenderTexture)` は GPU リソースの明示解放が必須で、GC 依存では回収タイミングが不定でリークする。特に `SwitchCamera` を繰り返すシナリオで GPU リソースが積み上がる。

## 現状

`SoraUnitySdkExamples/Assets/SoraUnitySdk/Sora.cs` の `Sora()` コンストラクタは次のように `CommandBuffer` を生成する:

```csharp
public Sora()
{
    p = sora_create();
    selfHandle = GCHandle.Alloc(this);
    commandBuffer = new UnityEngine.Rendering.CommandBuffer();
    sora_set_on_rpc(p, RpcCallback, GCHandle.ToIntPtr(selfHandle));
}
```

`Sora.Dispose()` は `sora_destroy` や adapter Dispose は行うが `commandBuffer.Release()` の呼び出しがない。

`Connect` と `SwitchCamera` は次のように `RenderTexture` を生成する:

```csharp
var texture = new UnityEngine.RenderTexture(config.CameraConfig.VideoWidth, config.CameraConfig.VideoHeight, config.CameraConfig.UnityCameraRenderTargetDepthBuffer, UnityEngine.RenderTextureFormat.BGRA32);
config.CameraConfig.UnityCamera.targetTexture = texture;
```

`SwitchCamera` では以下のようにするだけで、古い `RenderTexture` を `UnityEngine.Object.Destroy` しない:

```csharp
unityCamera.enabled = false;
unityCamera.targetTexture = null;
unityCamera = null;
```

Unity ドキュメント上、`CommandBuffer` と `RenderTexture` はいずれも明示的な破棄が必要とされている:

- `CommandBuffer.Release()` を呼ばないと、内部の native resource が GC のタイミングまで解放されない
- `RenderTexture` は `Release()` (GPU 側のバッファ) と `Object.Destroy` (C# 側の参照) の両方が必要

`SwitchCamera` を頻繁に繰り返すシナリオで GPU 側のバッファが積み上がる。

## 設計方針

- メンバフィールド (例: `RenderTexture? renderTexture`) を追加し、`Connect` / `SwitchCamera` で作成した `RenderTexture` を保持する。現状は `unityCamera.targetTexture` 経由でしか参照できず、次の切り替え・破棄の追跡手段がない
- `Sora.Dispose()` で `commandBuffer.Release()` を呼ぶ

```csharp
public void Dispose()
{
    ...
    if (commandBuffer != null)
    {
        commandBuffer.Release();
        commandBuffer = null;
    }
    ...
}
```

- `SwitchCamera` では、旧 `RenderTexture` の `Release()` / `Destroy()` を **`sora_switch_camera` の戻り後** に実行する

```csharp
// SwitchCamera 内
var oldTexture = renderTexture;  // メンバに保持した作成済み RenderTexture
var newTexture = (UnityCamera への切り替えの場合) 新規作成した RenderTexture : null;
... (cc 構築と sora_switch_camera 呼び出し) ...
if (oldTexture != null)
{
    oldTexture.Release();
    UnityEngine.Object.Destroy(oldTexture);
}
renderTexture = newTexture;  // 新しいテクスチャをメンバに保持し直す
```

- 破棄タイミングの順序契約: native の `Sora::SwitchCamera` は旧 `UnityCameraCapturer` を `Stop()` してから `capturer_` を置き換え、`f.wait()` で完了を待ってから戻る。したがって **`sora_switch_camera` の戻り後は native が旧テクスチャを参照しない**。`sora_switch_camera` を呼ぶ前に破棄すると旧キャプチャラがまだ旧テクスチャを参照しているため UAF になる。破棄は必ず戻り後に実行する
- この順序契約は、native の `Sora::SwitchCamera` が **return する時点で旧キャプチャラの `Stop()` を完了している**ことを不変条件とする。現在の成功経路はこれを満たす (Stop は post 前に同期実行される) が、既存の `if (!set_offer_) return;` の早期 return は Stop 前に戻るため満たさない。したがって `Sora::SwitchCamera` の全復帰経路 (既存の `!set_offer_` 早期 return、「SwitchCamera の future 待機によるデッドロックを解消する」issue (0029) の `ioc_->stopped()` 早期 return、タイムアウト、成功) で、return 前に旧キャプチャラの `Stop()` を実行するよう native 側を実装する。0029 も同じ不変条件を保持する (タイムアウト化については、Stop が post 前に同期実行されるため旧テクスチャの破棄タイミングは変わらない)
- `Connect` では旧 `RenderTexture` の破棄を行わない。native の `DoConnect` は旧キャプチャラを `Stop()` せず `capturer_` を置き換え、`sora_connect` には戻り後の同期点が存在しないため、Connect 時点での破棄は UAF になる。`Connect` では新規作成した RenderTexture をメンバに保持するだけとし、破棄は SwitchCamera / Dispose で行う (再接続時の旧テクスチャの破棄は本 issue の対象外)
- `Sora.Dispose()` では `sora_destroy` の後に (native の `~Sora` がキャプチャラを停止してから) 保持している `RenderTexture` を `Release()` / `Destroy()` する

## 完了条件

- `Sora.Dispose()` で `commandBuffer.Release()` が呼ばれる
- `SwitchCamera` で古い RenderTexture が `sora_switch_camera` の戻り後に Release / Destroy される
- `Sora::SwitchCamera` の全復帰経路で old capturer の Stop が return 前に完了し、接続確立前の SwitchCamera 呼び出し (早期 return) でも旧テクスチャの破棄で UAF にならない
- `Connect` / `SwitchCamera` で作成した RenderTexture がメンバに保持され、次の SwitchCamera / Dispose で破棄される
- `Sora.Dispose()` でも作成済み RenderTexture が Release / Destroy される
- SwitchCamera を 100 回程度繰り返しても GPU メモリ使用量が安定していることを確認する
- `CHANGES.md` の `## develop` に `[FIX] CommandBuffer と RenderTexture の Release/Destroy 漏れを修正する` を追記する
