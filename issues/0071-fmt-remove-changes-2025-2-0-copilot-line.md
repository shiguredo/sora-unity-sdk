# CHANGES.md 2025.2.0 misc の copilot-instructions.md 追加記述と実態の乖離を整理する

- Priority: Low
- Created: 2026-08-27
- Completed: 2026-09-11
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

## 解決方法

コード・`CHANGES.md` への変更は行わず closed にした。本 issue の前提（2025.2.0 の記述が実態と乖離しており修正が必要）は、`shiguredo-changelog` 規約と本リポジトリの慣行に照らすと成立しないため。

### 照合結果（実ファイル・git 履歴）

- `CHANGES.md` 266 行（2025.2.0 `### misc` の `[ADD] .github ディレクトリに copilot-instructions.md を追加`）は実在する。
- ただし、この記述は 2025.2.0 リリース時点の事実として正しい。リリースコミット `8647e30`（Merge tag '2025.2.0'、2025-08-26）時点で `.github/copilot-instructions.md` は実在し、`CHANGES.md` にも同エントリが存在する（`git show 8647e30:.github/copilot-instructions.md` と `git show 8647e30:CHANGES.md` で確認）。追加コミット `4492dc5`（copilot-instructions.md を追加、2025-06-22）がファイル追加とエントリ追記を同時に行っている。
- 削除コミット `a41fca0`（GitHub Copilot と Claude の設定ファイルを削除する、2026-06-09）は `.github/copilot-instructions.md` の削除のみで、`CHANGES.md` には一切触れていない（`git show --stat a41fca0` で確認。`git log -S copilot -- CHANGES.md` は `4492dc5` の 1 件のみ。2025.2.0 セクションと develop セクションの双方に削除記録は無い）。

### 修正不要である根拠

- `shiguredo-changelog` 規約にリリース済みセクションを後から書き換える規定は無く、本リポジトリの慣行はリリース済みセクションを歴史的記録として保持する。前例: `UseHardwareEncoder` は 2023.3.0 で `[ADD]`（現 437 行）、2025.2.0 で `[CHANGE]` の削除（現 191 行）となったが、2023.3.0 の `[ADD]` エントリは削除・注記されずそのまま残っている。
- 削除側の記録については、`shiguredo-changelog` 規約「`.rst` / `.md` ファイルの変更は変更履歴に反映しないこと」により、`copilot-instructions.md`（`.md` ファイル）の削除は `CHANGES.md` に記載してはならない。現状の「削除の追記が無い」は規約違反ではなく規約どおりの状態である。
- 本 issue の設計方針の 3 案はすべて規約・慣行に反する。
  - 「2025.2.0 misc の該当記述を削除する」: リリース済みセクションの歴史改変。記述は 2025.2.0 リリース時の事実であり、削除を指示する規約は無い。
  - 「削除された旨を追記する」: 同上。過去セクションへの後付け注記の前例は本リポジトリの `CHANGES.md` に存在しない。
  - 「`## develop` の misc にメモを追記する」: `.md` ファイル変更の記録禁止に違反する上、追加するものはエントリ形式 `- [種別] 変更内容...` でもない。develop セクションは未リリース変更の記録場所であり、過去バージョンに関するメモの置き場ではない。
- 「読み手が『まだ copilot-instructions.md がある』と誤解する」という前提も、`CHANGES.md` がリリース履歴（各リリース時点で何が変わったかを記録する文書）である以上成立しない。記述は「2025.2.0 で追加された」事実を述べているだけで、現在の存在を主張していない。

したがって、報告された「乖離」は `CHANGES.md` の修正対象となる欠陥ではなく、規約上の正しい状態である。issue の前提が崩壊しているため、対応不要として closed にする。

補足: 削除コミットは `151aaed`（`.github/workflows/claude.yml` の削除、同一メッセージ）と `a41fca0`（`.github/copilot-instructions.md` の削除）の 2 つに分かれており、`claude.yml` は追加時（`ed32204`）から一度も `CHANGES.md` に記録されていない。これも本 issue の範囲外である。
