# CHANGES.md 2025.2.0 misc の copilot-instructions.md 追加記述と実態の乖離を整理する

- Priority: Low
- Created: 2026-08-27
- Branch: fmt/remove-changes-2025-2-0-copilot-line
- Polished: {YYYY-MM-DD}

## 目的

`CHANGES.md` 2025.2.0 misc に残っている `.github` 配下の `copilot-instructions.md` を追加した旨の記述と、その後のコミットで実態が削除された事実との乖離を整理する。

## 現状

`CHANGES.md` 2025.2.0 の `### misc` セクションに `[ADD] .github ディレクトリに copilot-instructions.md を追加` の記述がある。

一方 `git log` を辿ると、追加コミットの後で `GitHub Copilot と Claude の設定ファイルを削除する` というコミットにより `.github/copilot-instructions.md` は削除済み。

CHANGES.md には削除の追記が無く、過去バージョンの記述が実態と乖離した状態で残っている。

過去バージョンの記述を後から書き換えるべきかどうかは shiguredo-changelog 規約と照らして判断する必要があるが、現状のままだと読み手が「まだ copilot-instructions.md がある」と誤解する。

## 設計方針

- `shiguredo-changelog` 規約を確認し、過去バージョンの記述を修正することが許容されるかを確認する
- 許容されるなら、2025.2.0 misc の該当記述を削除するか、削除された旨を追記する
- 許容されないなら、代替として `## develop` の misc に「過去バージョンで追加した copilot-instructions.md は既に削除済みである旨のメモを追記する」形にする
- 実装ファイルに手を入れる変更ではない

## 完了条件

- CHANGES.md の該当記述と実態が整合している
- shiguredo-changelog 規約に反しない形で修正されている
