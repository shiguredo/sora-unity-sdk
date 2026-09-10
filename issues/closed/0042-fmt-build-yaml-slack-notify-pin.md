# build.yml の slack-notify action をコミットハッシュ固定にする

- Priority: High
- Created: 2026-08-27
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
