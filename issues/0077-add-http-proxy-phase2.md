# HTTP Proxy 対応 Phase 2（OS 設定の自動参照）

- Created: 2026-09-10
- Completed: {YYYY-MM-DD}
- Branch: feature/add-http-proxy-phase2
- Polished: 2026-09-11

## 目的

Sora Unity SDK のプロキシ設定が未指定の場合に OS のプロキシ設定を自動参照し、手動設定なしでプロキシ経由の接続を可能にする。企業ネットワーク環境での設定コストを削減する。

## 現状

Phase 1 で `Sora.Config` からプロキシを手動指定できるが、OS のプロキシ設定は参照していない。

- `SoraUnitySdkExamples/Assets/SoraUnitySdk/Sora.cs` の `Sora.Config` が `ProxyUrl` / `ProxyUsername` / `ProxyPassword` / `ProxyAgent` を持つ
- `Sora.cs` の `Sora.Connect(Config)` が `cc.proxy_url` / `cc.proxy_username` / `cc.proxy_password` / `cc.proxy_agent` に設定し、ネイティブ側の `sora-cpp-sdk` の `SoraSignalingConfig` に渡す
- Unity SDK は Windows / macOS / Ubuntu / Android / iOS をサポートする（`README.md`）
- OS のプロキシ設定を読むコードは存在しない
- `sora-cpp-sdk` の `SoraSignalingConfig.proxy_url` が空の場合に OS のプロキシ設定を自動参照する仕組みはなく、プロキシは未指定なら適用されない

## 設計方針

- 変更対象は `SoraUnitySdkExamples/Assets/SoraUnitySdk/Sora.cs`、必要に応じて macOS / iOS 用のネイティブコード（`src/` 配下の C ABI と `.mm`）、および `CHANGES.md`
- OS プロキシの読み取りは Unity 側（C# を主体とし、Apple 系プラットフォームはこのリポジトリのネイティブコード）で行う。`Sora.Connect(Config)` が `cc.proxy_url` を設定する箇所で、`config.ProxyUrl` が空の場合に読み取った値を設定し、`Sora.Config` のフィールドは変更しない
- ネイティブの `sora-cpp-sdk` 側で OS プロキシを読む案は採用しない。対応する issue が `sora-cpp-sdk` に存在せず、全 SDK の挙動を変える設計判断と他リポジトリの作業が必要になるため、本 issue のスコープ外とする（必要になった場合は別 issue）
- プロキシの適用自体は Phase 1 と同じくネイティブの `sora-cpp-sdk` が担う。`SoraSignalingConfig.proxy_url` は WebSocket シグナリングと ICE（TURN を含む）の両方に適用される
- OS プロキシの読み取り元はプラットフォームごとに次のとおり
  - Windows: レジストリ `HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Internet Settings` の `ProxyEnable` / `ProxyServer`（WinINET のシステムプロキシ設定）
  - macOS / iOS: SystemConfiguration の `CFNetworkCopySystemProxySettings` が返す `HTTPProxy` / `HTTPPort` / `HTTPSProxy` / `HTTPSPort`。C# から直接取得できないため、このリポジトリのネイティブコード（C ABI）経由で取得する
  - Ubuntu: 環境変数 `http_proxy` / `https_proxy` / `all_proxy`（大文字小文字の両方を確認する）
  - Android: `ConnectivityManager.getDefaultProxy()`（または `getProxyForNetwork()`）が返す host / port。Android の Wi-Fi プロキシ設定には認証情報の入力欄が無いため、ユーザー名・パスワードは取得できない
- 読み取った host / port は `http://<host>:<port>` 形式に組み立てて設定する（`SoraSignalingConfig.proxy_url` は URL 形式を期待する）
- 明示設定（`ProxyUrl` 非空）を優先し、未指定時のみ OS 設定にフォールバックする
- OS のプロキシ設定が存在しない、または読み取れない場合は、プロキシなしで接続する（現在の既定の挙動を変えない）
- PAC ファイルの自動解決は本 issue のスコープ外とし、別 issue とする（JavaScript エンジンが必要）

## 完了条件

- `Config.ProxyUrl` が空で、OS のプロキシ設定が存在する環境（Windows / macOS / Ubuntu）で、WebSocket シグナリングと TURN を含む ICE がプロキシ経由で接続できること
- Android / iOS では、OS から読み取った host / port（認証情報を含まない）のプロキシとして接続できること。認証付きプロキシを利用する場合は、これまでどおり `Config.ProxyUrl` などの明示設定で指定すること
- `Config.ProxyUrl` を指定した場合は従来どおり明示設定が優先され、Phase 1 の挙動が変わらないこと
- `Config.ProxyUrl` が空かつ OS のプロキシ設定が存在しない環境では、プロキシなしで接続する（Phase 1 と同じ挙動）
- `CHANGES.md` の `## develop` に `[ADD]` エントリを追記すること
