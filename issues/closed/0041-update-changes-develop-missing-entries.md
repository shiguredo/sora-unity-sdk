# CHANGES.md の develop セクションに漏れている変更履歴を追記する

- Priority: High
- Created: 2026-08-27
- Completed: 2026-09-10
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

## 解決方法

コード変更は行わず closed にした。報告された「漏れている変更履歴」（`SoraAndroidDependencyInjector` 削除と `BOOST_ASIO_DISABLE_STD_ATOMIC_WAIT` 削除のエントリ欠落）は、git 履歴の実ファイル照合では存在しないため。

git 履歴での照合結果（いずれも `2026.1.0` リリース（2026-02-18、`4d06fee`）以降、未リリースの develop のみで完結している）:

- `SoraAndroidDependencyInjector` は `8ba27ce`（2026-06-22）で `## develop` に `[ADD]` エントリ付きで追加され、`ecd76ab`（2026-06-24）で削除された。この削除コミットは `CHANGES.md` から該当 `[ADD]` エントリも同時に削除しており、`## develop` は追加前の状態へ戻っている
- `BOOST_ASIO_DISABLE_STD_ATOMIC_WAIT` は `f422393`（2026-06-11、canary.14）の `[UPDATE]` エントリの子項目として追加され、`beefe9d`（2026-06-25）で子項目ごと削除された。`b7343dc`（2026-06-25）はコメント・全角半角の書式修正のみ
- 両変更ともリリース済みバージョンへは一切含まれていない（`CHANGES.md` に `## 2026.2.0` セクションは存在せず、`2026.2.0` タグも無い。最新リリースは `2026.1.0`）

したがって、`shiguredo-changelog` 規約の「派生元ブランチとの最終的な差分のみを記載する」「開発ブランチ内の中間状態の修正は記載しない」に従えば、最終差分にはどちらの変更も現れず、エントリを追記すべき箇所は無い。issue の設計方針どおりに `[CHANGE]` / `[UPDATE]` を追記すると、むしろ規約違反の記述を `CHANGES.md` へ持ち込むことになる。`8274a9b`（不要なガードの整理）も未リリースファイルの内部整理であり、記録対象外である。

補足: 完了条件「直近 30 コミット全てが `## develop` セクションのいずれかの項目でカバーされている」は、直近のコミットの大部分が issue 管理コミット（issue ファイル・SEQUENCE のみの変更）であり、`shiguredo-changelog` の「`.md` ファイルの変更は変更履歴に反映しない」で記録対象外となるため、そのままでは検証できない基準である。

備考: polish-issue のレビューで本件が処理不能指摘（報告された問題の不存在。git 履歴の実ファイル照合で確定）となったため、`Polished:` は更新していない。
