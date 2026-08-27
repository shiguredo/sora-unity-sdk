# CommandBuffer と RenderTexture の Release/Destroy 漏れを修正する

- Priority: High
- Created: 2026-08-27
- Branch: fix/command-buffer-render-texture-leak
- Polished: {YYYY-MM-DD}
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

- `SwitchCamera` および `Connect` で古い `RenderTexture` を保持し、切り替え時に `Object.Destroy` する

```csharp
// SwitchCamera 内
var oldTexture = unityCamera.targetTexture as UnityEngine.RenderTexture;
unityCamera.targetTexture = null;
if (oldTexture != null)
{
    oldTexture.Release();
    UnityEngine.Object.Destroy(oldTexture);
}
```

- `Sora.Dispose()` でも `Connect` 時に作成した RenderTexture がまだ生きていれば Destroy する
- `SwitchCamera` と C++ 側 `Sora::SwitchCamera` の順序を見直し、native がまだ古いテクスチャポインタを触っている間に C# 側で Destroy しないように順序契約を明示する (別 issue の SwitchCamera race と併せて設計する)

## 完了条件

- `Sora.Dispose()` で `commandBuffer.Release()` が呼ばれる
- `SwitchCamera` で古い RenderTexture が Release / Destroy される
- `Connect / SwitchCamera` を 100 回程度繰り返しても GPU メモリ使用量が安定していることを確認する
- `CHANGES.md` の `## develop` に `[FIX] CommandBuffer と RenderTexture の Release/Destroy 漏れを修正する` を追記する
