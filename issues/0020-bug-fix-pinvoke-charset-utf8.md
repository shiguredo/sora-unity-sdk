# P/Invoke に CharSet を明示指定して Windows での UTF-8 文字列破壊を修正する

- Priority: Critical
- Created: 2026-08-27
- Branch: fix/pinvoke-charset-utf8
- Polished: {YYYY-MM-DD}
- Milestone: 2026.2.0

## 目的

`SoraUnitySdkExamples/Assets/SoraUnitySdk/Sora.cs` の全 `[DllImport]` と `[UnmanagedFunctionPointer]` が `CharSet` を明示指定していない。Windows Mono の既定は `CharSet.Ansi` (システム ANSI コードページ、日本語 Windows なら CP932) で string と char* を変換するが、ネイティブ側 `sora.cpp` は `std::string` の UTF-8 バイト列を返す。日本語や絵文字を含むチャンネル ID / metadata / label / connection_id は Windows で mojibake になる。macOS / Ubuntu / Android では Mono の Ansi が UTF-8 相当なので露見しにくく、Windows 検証が薄いため見逃されている。

## 現状

`SoraUnitySdkExamples/Assets/SoraUnitySdk/Sora.cs` 内の 40 件超の `[DllImport(DllName)]` はすべて次のような形で書かれており、`CharSet` の指定がない:

```csharp
[DllImport(DllName)]
static extern void sora_send_message(IntPtr p, string label, byte[] buf, int size);
```

同ファイル内の各種コールバック delegate 定義 (例: `TrackCallbackDelegate` / `MessageCallbackDelegate` / `RpcCallbackDelegate`) にも `[UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Ansi)]` のような CharSet 指定がない:

```csharp
private delegate void RpcCallbackDelegate(string json, IntPtr userdata);
```

.NET / Unity Mono の `DllImport` 既定は `CharSet.Ansi`。Windows Mono では現在のシステム ANSI コードページ (日本語 Windows で CP932) で `string` <-> `char*` を変換する。

一方で C++ 側は `std::string` を通じて UTF-8 バイト列を扱っており、`c_str()` で得たポインタもそのまま UTF-8 として想定している。往路 (C# -> native) では UTF-8 の JSON を CP932 で解釈することになり、復路 (native -> C#) では CP932 で解釈された文字列を返すため、非 ASCII 文字を含む値は必ず mojibake する。

macOS / Ubuntu / Android の Mono では Ansi が UTF-8 相当として扱われるため露見しづらいが、Windows のみで見えない不具合として残り続けている。

## 設計方針

- 全 `[DllImport(DllName)]` に `CharSet` を明示指定する
  - `CharSet.Ansi` を明示するだけでは Windows での挙動は変わらないため、UTF-8 マーシャリングを実現する仕組みが必要
- Unity 2021.2 以降で利用可能な `[LibraryImport]` (.NET 7 の source-generated P/Invoke) の UTF-8 マーシャラを使う経路を検討する
  - `[MarshalAs(UnmanagedType.LPUTF8Str)]` を各 `string` 引数と戻り値に付与する
- 現実的な移行方針として、各 `[DllImport]` に対して以下いずれかの対応を採る
  - (a) `[DllImport(DllName, CharSet = CharSet.Ansi)]` にした上で、各 `string` 引数と戻り値に `[MarshalAs(UnmanagedType.LPUTF8Str)]` を付与
  - (b) `string` を `byte[]` (UTF-8 バイト列) に変更し、C# 側で `Encoding.UTF8` を使って手動マーシャリング
- delegate 定義にも `[UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Ansi)]` を付与し、必要に応じて `[MarshalAs(UnmanagedType.LPUTF8Str)]` を付ける
- Sora.cs 内で string を扱う全境界を体系的に洗い出し、抜け漏れが無いことを保証する

## 完了条件

- `SoraUnitySdkExamples/Assets/SoraUnitySdk/Sora.cs` の全 `[DllImport]` と全 delegate 定義に UTF-8 マーシャリングが施されている
- Windows 日本語環境で日本語文字列を含む channelId / metadata / label / connection_id が正しく送受信されることを確認する
- macOS / Ubuntu / Android で既存の挙動に regression が発生していないことを確認する
- リリース検証項目 (issues/0006 系) に Windows の日本語文字列疎通確認を追加する
- `CHANGES.md` の `## develop` に `[FIX] P/Invoke に UTF-8 マーシャリングを明示指定して Windows での日本語文字列破壊を修正する` を追記する
