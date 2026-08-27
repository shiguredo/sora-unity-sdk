# build.yml の cache restore-keys から未定義参照を削除する

- Priority: High
- Created: 2026-08-27
- Branch: fix/build-yaml-restore-keys
- Polished: {YYYY-MM-DD}
- Milestone: 2026.2.0

## 目的

`.github/workflows/build.yml` の cache ステップの `restore-keys` が未定義の matrix プロパティを参照しており、実質的にキャッシュ復元がほぼ機能していない。CI が毎回 fresh install を強いられ、ビルド時間が肥大化する原因になっているため修正する。

## 現状

`.github/workflows/build.yml` の cache ステップは次のように書かれている。

- `key: ${{ matrix.target }}-v1-${{ hashFiles('VERSION', 'DEPS') }}`
- `restore-keys: | ${{ matrix.name }}-v1-`

問題点:

- matrix には `target` と `runs-on` しか定義されておらず、`matrix.name` は存在しない
- `${{ matrix.name }}` は空文字列として展開され、`restore-keys` は事実上 `-v1-` プレフィックスのみを検索する
- ほとんどのケースでキャッシュヒットせず、依存の再取得やビルドが毎回走る
- CI 全体の実行時間が伸び、canary リリースのペースが落ちる

## 設計方針

- `restore-keys` を `${{ matrix.target }}-v1-` に修正する
- key と同じベース prefix に揃えることで、`hashFiles` の結果が変わってもプラットフォーム別に部分ヒットするようにする
- 併せて `key` と `restore-keys` の設計意図をコメントで残す（残す価値がある場合のみ）

## 完了条件

- `restore-keys` が `${{ matrix.target }}-v1-` を参照している
- 主要ブランチで cache の hit / miss 統計が改善している
- CI ビルド時間が短縮している
- `CHANGES.md` の `## develop` の misc に `[FIX]` を追記する
