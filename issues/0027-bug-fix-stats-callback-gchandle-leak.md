# StatsCallback の GCHandle 例外時リークを修正する

- Priority: High
- Created: 2026-08-27
- Branch: fix/stats-callback-gchandle-leak
- Polished: 2026-08-31
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

コールバックの GCHandle には所有モデルが 2 種類ある。

- 呼び出しごとに `GCHandle.Alloc` するモデル: `GetStats` → `StatsCallback` (コールバック内で Free) と、`GetVideoCapturerDevices` / `GetAudioRecordingDevices` / `GetAudioPlayoutDevices` → `DeviceEnumCallback` (呼び出し元メソッドで Free)。どちらも例外パスで Free がスキップされると GCHandle と Action がリークする
- インスタンス単位で 1 回 `GCHandle.Alloc` するモデル: `DisconnectCallback` / `AddTrackCallback` / `RemoveTrackCallback` / `MessageCallback` など 13 個のコールバックが `selfHandle` (コンストラクタで Alloc、`Dispose` でのみ Free) を共有する。これらのコールバックには Free する経路がなく、例外が起きても GCHandle はリークしない

また、現在の `StatsCallback` の形 (try/finally なし + `callback!(...)`) は 2025.3.0 の nullable 対応 (「nullable 対応を見直し」コミット) で導入された。CHANGES.md 2025.3.0 に「コールバック呼び出しを `callback!(...)` に統一する」と記録されているため、本 issue ではコールバック呼び出しの形式を維持したまま、GCHandle の解放だけを確実化する。

## 設計方針

- `StatsCallback` を try / finally で `handle.Free()` が確実に実行される形に変更する

```csharp
[AOT.MonoPInvokeCallback(typeof(StatsCallbackDelegate))]
static private void StatsCallback(string json, IntPtr userdata)
{
    GCHandle handle = GCHandle.FromIntPtr(userdata);
    try
    {
        var callback = handle.Target as Action<string>;
        callback!(json);
    }
    finally
    {
        handle.Free();
    }
}
```

- `GCHandle.FromIntPtr` の呼び出しを 1 回に統一する (`handle` 変数を再利用する)
- コールバック呼び出しは CHANGES.md 2025.3.0 で統一された `callback!(...)` のまま変更しない。null が渡るのは呼び出し側のバグであり、その場合も NRE が発生するが、finally で `handle.Free()` は実行されるためリークしない。例外は catch せず伝播させる (旧実装の `Debug.LogException` は復活させない)
- `DeviceEnumCallback` 系 (`GetVideoCapturerDevices` / `GetAudioRecordingDevices` / `GetAudioPlayoutDevices`) は、呼び出し元メソッドの `handle.Free()` を try / finally に移し、`DeviceEnumCallback` 内で例外が起きても Free が実行されるようにする
- `selfHandle` パターンのコールバック (DisconnectCallback / AddTrackCallback / RemoveTrackCallback / MessageCallback など 13 個) は対象外とする。これらのコールバックには Free する経路がなく例外時にもリークしないため、`StatsCallback` と同様の `handle.Free()` を追加すると共有 selfHandle の二重解放になる

## 完了条件

- `StatsCallback` が例外パス (コールバックの例外、null 時の NRE を含む) でも `handle.Free()` を実行し、GCHandle をリークしない
- `GetStats` を長時間繰り返しても GCHandle が積み上がらないことを確認する
- `DeviceEnumCallback` 系 (`GetVideoCapturerDevices` / `GetAudioRecordingDevices` / `GetAudioPlayoutDevices`) がコールバック例外パスでも `handle.Free()` を実行する
- `selfHandle` パターンのコールバックに `handle.Free()` が追加されていない (二重解放の防止)
- 同じ userdata で `GCHandle.FromIntPtr` を 2 回呼ぶ無駄が解消されている
- `CHANGES.md` の `## develop` に `[FIX] StatsCallback の GCHandle 例外時リークを修正する` を追記する
