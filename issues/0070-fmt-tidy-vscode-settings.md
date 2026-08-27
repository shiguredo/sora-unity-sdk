# .vscode/settings.json の巨大な files.associations マップを整理する

- Priority: Low
- Created: 2026-08-27
- Branch: fmt/tidy-vscode-settings
- Polished: {YYYY-MM-DD}

## 目的

`.vscode/settings.json` に含まれる 100 行超の `files.associations` マップを整理し、リポジトリに残す必要のあるエントリだけに絞る。

## 現状

`.vscode/settings.json` の `files.associations` には C++ 標準ライブラリと WebRTC / Sora 関連ヘッダの 100 行以上のマッピングが列挙されている。

これらは Visual Studio Code の IntelliSense がヘッダ拡張子を判定するためのマッピングだが、`.clang-format` と `c_cpp_properties.json` が整備されている環境では大半が VSCode 標準機能で解決可能。個人の作業環境で追加されたマッピングがそのままコミットされている痕跡がある。

リポジトリの `.vscode/` に置く共通設定としては肥大化しすぎており、Git のマージコンフリクト源にもなりやすい。

## 設計方針

- `files.associations` を必要最低限のエントリに絞る（VSCode 標準で判定できないもののみ）
- 開発者ごとの個別マッピングは `.vscode/settings.json` から外し、必要なら各人の `settings.json` に置く
- リポジトリに残す共通設定の粒度を README か CONTRIBUTING に明記する
- 挙動変更は VSCode 内での補助機能のみ

## 完了条件

- `.vscode/settings.json` の `files.associations` が半分以下のサイズに整理されている
- 標準 IntelliSense で解決可能なエントリが除外されている
- リポジトリのビルド動作に影響しない
