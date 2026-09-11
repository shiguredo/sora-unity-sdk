# .vscode/settings.json の巨大な files.associations マップを削除する

- Priority: Low
- Created: 2026-08-27
- Branch: fmt/tidy-vscode-settings
- Polished: 2026-09-11

## 目的

`.vscode/settings.json` に含まれる 100 エントリの `files.associations` マップを削除し、リポジトリ共通設定を最小限に保つ。

## 現状

`.vscode/settings.json` の `files.associations` には 100 エントリのマッピングが列挙されている。内訳は次のとおり。

- ファイルパターン 2 件: `"*.cs": "csharp"`、`"*.ipp": "cpp"`
- 拡張子なしの C++ 標準ライブラリヘッダと MSVC 実装内部ヘッダ 98 件 (`memory`, `utility`, `thread`, `xmemory`, `xhash` など。`hash_map` / `hash_set` / `resumable` は MSVC 固有)

WebRTC / Sora 関連ヘッダのエントリは含まれていない。

`git log --follow -- .vscode/settings.json` を辿ると、初期コミット (`a8e6919`) で 98 エントリが含まれており、2019-11-13 の無関係なコミット (`c86a310`) で `hash_map` / `hash_set` が追加されている。C/C++ 拡張 (ms-vscode.cpptools) は Go to Definition で拡張子なしヘッダを開いた際に `files.associations` へ自動追加していたため (microsoft/vscode-cpptools#722)、個人の作業環境で自動生成されたマッピングがそのままコミットされた痕跡である。

一方、次のとおりリポジトリに残す必要のあるエントリは存在しない。

- VSCode 組み込み言語定義は `*.cs` (csharp) と `*.ipp` (cpp) を既に解決する (microsoft/vscode `extensions/csharp/package.json` と `extensions/cpp/package.json` の `contributes.languages`)
- C/C++ 拡張は v1.29.2 で拡張子なしのシステムヘッダ向けの組み込みファイル連想を追加し、`C_Cpp.autoAddFileAssociations` のデフォルトを `false` に変更した (microsoft/vscode-cpptools#4077)

なお、`.clang-format` と `c_cpp_properties.json` は言語モードの判定には関与しない (`c_cpp_properties.json` は IntelliSense の include path 設定であり、`files.associations` の代替にはならない)。

リポジトリの `.vscode/` に置く共通設定としては肥大化しすぎており、Git のマージコンフリクト源にもなりやすい。

## 設計方針

- `files.associations` マップ全体を `.vscode/settings.json` から削除する
- 開発者ごとの個別マッピングは `.vscode/settings.json` から外し、必要なら各人の `settings.json` に置く
- `files.associations` 以外の設定 (`python.analysis.autoImportCompletions`、`python.analysis.typeCheckingMode`、`[python]`、`[cpp]` など) は変更しない
- 前提は C/C++ 拡張 v1.29.2 以降 (2025 年 12 月リリース) を使用する環境であること
- 挙動変更は VSCode 内での補助機能のみ

## 完了条件

- `.vscode/settings.json` から `files.associations` が削除されている
- `.vscode/settings.json` の他の設定が維持されている
- リポジトリのビルド動作に影響しない
