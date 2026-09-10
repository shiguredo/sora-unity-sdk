# CHANGES.md の develop セクションに漏れている変更履歴を追記する

- Priority: High
- Created: 2026-08-27
- Branch: update/changes-develop-missing-entries
- Polished: {YYYY-MM-DD}
- Milestone: 2026.2.0

## 目的

`CHANGES.md` の `## develop` セクションに、develop ブランチにマージ済みの変更が記載されていない。shiguredo-changelog 規約違反であり、リリース時に破壊的変更を利用者へ告知できない。漏れている項目を追記する。

## 現状

直近のコミット履歴を確認すると、以下の変更が develop に取り込まれているが `CHANGES.md` の `## develop` セクションに記載が無い。

- `SoraAndroidDependencyInjector` 削除
  - コミット: `ecd76ab 不要になった SoraAndroidDependencyInjector を削除する`
  - コミット: `8274a9b 不要なガードを削除して SoraAndroidDependencyInjector.cs を整理する`
  - 公開クラスの削除であり、`[CHANGE]` 相当の破壊的変更
- `BOOST_ASIO_DISABLE_STD_ATOMIC_WAIT` 削除
  - コミット: `beefe9d BOOST_ASIO_DISABLE_STD_ATOMIC_WAIT を削除する`
  - コミット: `b7343dc BOOST_ASIO_DISABLE_STD_ATOMIC_WAIT 関連のコメントと書式を修正する`
  - ビルド動作に影響するビルドフラグ変更であり、`[UPDATE]` 相当

いずれも shiguredo-changelog 規約に従い、`## develop` セクションで告知する必要がある。

## 設計方針

- `## develop` セクションの本編に `[CHANGE] SoraAndroidDependencyInjector を削除する` を追記する
- `### misc` セクションに `[UPDATE] BOOST_ASIO_DISABLE_STD_ATOMIC_WAIT を削除する` を追記する
- 記述順は shiguredo-changelog 規約に従う
- 併せて漏れている他の項目が無いか直近 30 コミット程度を確認する

## 完了条件

- `## develop` セクションに `SoraAndroidDependencyInjector` 削除の `[CHANGE]` が記載されている
- `### misc` に `BOOST_ASIO_DISABLE_STD_ATOMIC_WAIT` 削除の `[UPDATE]` が記載されている
- 直近 30 コミット全てが `## develop` セクションのいずれかの項目でカバーされている
