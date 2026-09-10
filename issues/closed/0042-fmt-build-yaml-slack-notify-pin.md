# build.yml の slack-notify action をコミットハッシュ固定にする

- Priority: High
- Created: 2026-08-27
- Completed: 2026-09-10
- Branch: fmt/build-yaml-slack-notify-pin
- Polished: {YYYY-MM-DD}
- Milestone: 2026.2.0

## 目的

`.github/workflows/build.yml` の `slack-notify` action だけがブランチ参照 `@main` で固定されておらず、shiguredo-github-actions 規約に反している。他の action と同じ `owner/repo@<commit hash> # vX.Y.Z` 形式に揃える。

## 現状

`.github/workflows/build.yml` の他の action は次のように shiguredo-github-actions 規約に沿って書かれている。

- `actions/checkout@3d3c42e5aac5ba805825da76410c181273ba90b1 # v7.0.1`
- `actions/cache@55cc8345863c7cc4c66a329aec7e433d2d1c52a9 # v6.1.0`
- `actions/upload-artifact@043fb46d1a93c77aae656e7c1c64a875d1fc6a0a # v7.0.1`
- `actions/download-artifact@3e5f45b2cfb9172054b4087a40e8e0b5a5461e7c # v8.0.1`

しかし slack-notify のみ次のように `@main` を参照している。

- `shiguredo/github-actions/.github/actions/slack-notify@main`

問題点:

- コミットハッシュ固定になっていないため、shiguredo/github-actions 側の main が更新されると CI 挙動が予告なく変わる
- バージョンコメントが無く、どの版を使っているのか追跡できない
- shiguredo-github-actions 規約違反

## 設計方針

- `shiguredo/github-actions` の最新リリースタグと対応するコミットハッシュを取得する
- `shiguredo/github-actions/.github/actions/slack-notify@<hash> # vX.Y.Z` の形式に置き換える
- 以降 update-actions フローで自動更新できる形にする

## 完了条件

- `slack-notify` の参照がコミットハッシュ + バージョンコメントに揃っている
- 他の action と同じ形式である
- shiguredo-github-actions 規約に準拠している
- `CHANGES.md` の `## develop` の misc に `[UPDATE]` として反映する

## 解決方法

コード変更は行わず closed にした。issue の前提（`slack-notify` のみが shiguredo-github-actions 規約違反であり、他の action と同じコミットハッシュ固定 + バージョンコメントに揃えるべき）が、実ファイル・一次資料・git 履歴・GitHub API・他リポジトリの照合で成立しないため。

照合結果（2026-09-10 時点、`develop`）:

- **該当規約の不存在**: `shiguredo-github-actions` SKILL.md にはアクションのコミットハッシュ固定に関する規約が無い。内容は「action の選定」（GitHub 公式優先・許可済み外部 action 一覧・利用実績の確認）「runner の選定」「Rust toolchain の準備」のみであり、「shiguredo-github-actions 規約違反」の根拠となる記述は存在しない
- **ピン留め形式の正本が逆方向**: コミットハッシュ固定 + バージョンコメントの形式を定めているのは `update-actions` SKILL.md であり、同スキルは「ブランチ参照（ハッシュ固定なしの `@main` / `@master` 等）は、意図的に追従運用しているもの（例: `shiguredo/github-actions` の内部 action）があるため **書き換えない**。一覧表示フェーズで「対象外（ブランチ参照）」として報告するだけにする」（update-actions SKILL.md）と明記し、その例として `shiguredo/github-actions/.github/actions/slack-notify@main`（ブランチ参照。意図的な追従運用のため変更しない）を挙げている。本 issue の変更は同スキルの明示的な対象外判断に抵触する
- **バージョン不存在**: `shiguredo/github-actions` にはリリースが 0 件、タグが 0 件（GitHub API で確認）。設計方針の「最新リリースタグと対応するコミットハッシュを取得する」「`@<hash> # vX.Y.Z` の形式に置き換える」は、参照先にバージョン自体が存在しないため実現できない
- **git 履歴上の意図**: `446e2e6`「GitHub Actions の参照をコミット SHA に固定する」（2026-05-19）は外部 action（checkout / cache / upload-artifact / download-artifact）を一律 SHA 固定した一方、`slack-notify@main` だけは変更していない。追従運用として意図的に残したと見るのが整合的である
- **組織全体での一貫性**: 他 shiguredo リポジトリでも `shiguredo/github-actions` 配下の action（slack-notify / setup-cuda-toolkit / download-openh264 等）は全てブランチ参照 `@main` のままである（sora-cpp-sdk / momo / sora-android-sdk / sora-ios-sdk / sora-python-sdk / sora-devtools のワークフローを確認。sora-python-sdk のみ `@main # main` とコメント付きだが参照はブランチのまま）。本リポジトリだけが逸脱しているという前提も誤りである

事実記述の正しさ: 現状の記述（`.github/workflows/build.yml` の slack-notify のみが `@main` であり、他の action はハッシュ固定 + バージョンコメント）自体は実ファイルと一致する（checkout / cache / upload-artifact / download-artifact の各参照は行 31 / 34 / 47 / 59 / 69 / 80 / 82、slack-notify は行 105）。しかし「問題点」として挙げられた 3 点（CI 挙動が予告なく変わる・どの版を使っているか追跡できない・規約違反）は、上記の組織的運用方針の下では問題ではなく、「規約違反」の主張は誤りである。

以上より、設計方針どおりの実装は参照先にバージョンが存在せず不可能であり、かつ明示的な対象外運用への変更は規約・他リポジトリの一貫性に反する。これ以上の対応は不要と判断し closed にする。

備考: polish-issue のレビューで本件が処理不能指摘（前提崩壊。実ファイル・一次資料・git 履歴・GitHub API・他リポジトリの照合で確定）となったため、`Polished:` は更新していない。
