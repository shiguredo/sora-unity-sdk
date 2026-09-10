# HTTP Proxy 対応 Phase 2（OS 設定の自動参照）

- Created: 2026-09-10
- Completed: {YYYY-MM-DD}
- Branch: feature/add-http-proxy-phase2
- Polished: {YYYY-MM-DD}

## 目的

Sora Unity SDK のプロキシ設定が未指定の場合に OS のプロキシ設定を自動参照し、手動設定なしでプロキシ経由の接続を可能にする。企業ネットワーク環境での設定コストを削減する。

## 現状

Phase 1 で `SoraConfig` からプロキシを手動指定できるが、OS のプロキシ設定は参照していない。

- `SoraUnitySdkExamples/Assets/SoraUnitySdk/Sora.cs` の `SoraConfig` が `ProxyUrl` / `ProxyUsername` / `ProxyPassword` / `ProxyAgent` を持つ
- `Sora.cs` の `Sora` のコンストラクタが `cc.proxy_url` / `cc.proxy_username` / `cc.proxy_password` / `cc.proxy_agent` に設定し、ネイティブ側の `sora-cpp-sdk` の `SoraSignalingConfig` に渡す
- Unity SDK は Windows / macOS / Ubuntu / Android / iOS をサポートする（`README.md`）
- OS のプロキシ設定を読むコードは存在しない

## 設計方針

- `ProxyUrl` が空の場合に OS のプロキシ設定を参照する
- プロキシの適用はネイティブの `sora-cpp-sdk` が担うため、OS 設定の読み取りは次のどちらかとする
  - `sora-cpp-sdk` の Phase 2 に寄せる（別 issue）
  - Unity の C# 側でプラットフォームごとに読み取り、`SoraConfig.ProxyUrl` に設定する
- Unity SDK がサポートする全プラットフォーム（Windows / macOS / Ubuntu / Android / iOS）の動作を考慮する
- PAC ファイルの自動解決は本 issue のスコープ外とし、別 issue とする（JavaScript エンジンが必要）
- 明示設定（`ProxyUrl` 非空）を優先し、未指定時のみ OS 設定にフォールバックする

## 完了条件

- `ProxyUrl` 未指定でも、OS のプロキシ設定がある環境で WebSocket と TURN がプロキシ経由で接続できること
- `ProxyUrl` を指定した場合は従来どおり明示設定が優先され、Phase 1 の挙動が変わらないこと
- `CHANGES.md` の `## develop` に `[ADD]` エントリを追記すること
