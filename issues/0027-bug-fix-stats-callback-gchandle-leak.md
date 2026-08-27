# StatsCallback の GCHandle 例外時リークを修正する

- Priority: High
- Created: 2026-08-27
- Branch: fix/stats-callback-gchandle-leak
- Polished: {YYYY-MM-DD}
- Milestone: 2026.2.0

## 目的

`Sora.cs` の `StatsCallback` は `callback!(json)` が例外を投げると `handle.Free()` に到達せず GCHandle と Action がリークする。`GetStats` はサンプル `SoraSample.cs` で 10 秒周期に呼ばれる想定のため、長時間運用で確実なリーク源になる。加えて同じ userdata で `GCHandle.FromIntPtr` を 2 回呼ぶ無駄コードも整理する。

## 現状

`SoraUnitySdkExamples/Assets/SoraUnitySdk/Sora.cs` の `StatsCallback` は次のように書かれている:

```csharp
[AOT.MonoPInvokeCallback(typeof(StatsCallbackDelegate))]
static private void StatsCallback(string json, IntPtr userdata)
{
    GCHandle handle = GCHandle.FromIntPtr(userdata);
    var callback = GCHandle.FromIntPtr(userdata).Target as Action<string>;
    callback!(json);
    handle.Free();
}
```

問題点:

- `callback!(json)` が例外を投げると次行の `handle.Free()` に到達しない
- 結果として `GCHandle` (ネイティブ側と Managed 側の参照ペア) がリークし続ける
- 呼び出し側 `GetStats` は次のように毎回 `GCHandle.Alloc` するため、10 秒周期で GCHandle が積み上がる

```csharp
public void GetStats(Action<string> onGetStats)
{
    GCHandle handle = GCHandle.Alloc(onGetStats);
    sora_get_stats(p, StatsCallback, GCHandle.ToIntPtr(handle));
}
```

- 同じ `userdata` で `GCHandle.FromIntPtr(userdata)` を 2 回呼んでおり、コード的にも無駄
- `Target as Action<string>` の結果が null だった場合 `callback!` は NRE を投げる

## 設計方針

- `try / finally` で `handle.Free()` を確実化する

```csharp
[AOT.MonoPInvokeCallback(typeof(StatsCallbackDelegate))]
static private void StatsCallback(string json, IntPtr userdata)
{
    GCHandle handle = GCHandle.FromIntPtr(userdata);
    try
    {
        var callback = handle.Target as Action<string>;
        callback?.Invoke(json);
    }
    finally
    {
        handle.Free();
    }
}
```

- `GCHandle.FromIntPtr` の呼び出しを 1 回に統一する
- `callback` が null の場合の防御を追加する
- 他の `[AOT.MonoPInvokeCallback]` 経由コールバック (`DisconnectCallback` / `TrackCallback` / `MessageCallback` など) も同様に見直し、Managed 例外で GCHandle がリークしないパターンで書き直す

## 完了条件

- `StatsCallback` が例外パスで GCHandle をリークしない
- `GetStats` を長時間繰り返しても GC 情報上 GCHandle が積み上がらないことを確認する
- 同じ userdata で `GCHandle.FromIntPtr` を 2 回呼ぶ無駄が解消されている
- 他のコールバックにも同じパターンが適用されているか設計統一されている
- `CHANGES.md` の `## develop` に `[FIX] StatsCallback の GCHandle 例外時リークを修正する` を追記する
