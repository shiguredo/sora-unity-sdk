# build.yml の cache restore-keys の未定義参照を修正する

- Priority: High
- Created: 2026-08-27
- Branch: feature/fix-build-yaml-restore-keys
- Polished: 2026-09-10
- Milestone: 2026.2.0

## 目的

`.github/workflows/build.yml` の cache ステップの `restore-keys` が matrix に存在しない `${{ matrix.name }}` を参照しており、常に miss するため修正する。`VERSION` または `DEPS` の更新で `key` が変わったときに部分ヒットできず、依存の再インストールが走るのを防ぐ。

## 現状

`.github/workflows/build.yml` の cache ステップは次のように書かれている。

- `key: ${{ matrix.target }}-v1-${{ hashFiles('VERSION', 'DEPS') }}`
- `restore-keys: | ${{ matrix.name }}-v1-`

問題点:

- matrix には `target` と `runs-on` しか定義されておらず、`matrix.name` は存在しない
- `${{ matrix.name }}` は空文字列として展開され、`restore-keys` は事実上 `-v1-` プレフィックスのみを検索する
- キャッシュキーは `<target>-v1-<hash>` の形式のため、`-v1-` はどのキーの先頭にも一致せず、`restore-keys` による復元は常に miss する
- `VERSION` と `DEPS` が変わらない間は `key` の完全一致でキャッシュは復元されるが、どちらかが更新されて `key` が変わった直後の実行は miss し、依存の再インストールが走る

## 設計方針

- `restore-keys` を `${{ matrix.target }}-v1-` に修正する
- `key` と同じベース prefix に揃えることで、`hashFiles` の結果が変わってもプラットフォーム別に部分ヒットするようにする
- 合わせて、`key` と `restore-keys` の prefix を揃えている理由をコメントで残す

## 完了条件

- `restore-keys` が `${{ matrix.target }}-v1-` を参照している
- `VERSION` または `DEPS` が更新された実行で、前回の `<target>-v1-` キャッシュが復元される（actions/cache のログに `Cache restored from key` が出力される）
- `CHANGES.md` の `## develop` の `### misc` に `[FIX]` を追記する
