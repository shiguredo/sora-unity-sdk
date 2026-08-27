# SoraSample の日本語ログを英語に統一する

- Priority: High
- Created: 2026-08-27
- Branch: fmt/translate-sample-log-messages
- Polished: {YYYY-MM-DD}
- Milestone: 2026.2.0

## 目的

`SoraUnitySdkExamples/Assets/SoraSample.cs` に日本語のログ出力が残っており、AGENTS.md の「ログメッセージは全て英語にすること」規約に違反している。サンプルはテストではないため英語ログに統一する。併せてサンプル内の英語コメントを日本語化し、コメントと log の言語規約を揃える。

## 現状

`SoraSample.cs` に次のような日本語ログが残っている。

- `Debug.LogError("シグナリング URL が設定されていません")`
- `Debug.LogError("チャンネル ID が設定されていません")`
- `Debug.Log("RPC メッセージの種類が選択されていません")`
- `Debug.LogErrorFormat("RPC timeoutMillis の形式が不正です: ...")`

同時に、サンプル内には次のような英語コメントが残っており、こちらは AGENTS.md の「コメントは全て日本語にすること」規約に違反している。

- `// Start is called before the first frame update`
- `// Update is called once per frame`

## 設計方針

- 日本語ログを英語に置き換える（Debug.LogError / Debug.Log / Debug.LogErrorFormat の全メッセージ）
- 英語コメントを日本語に置き換える
- 変数名や識別子は変更しない
- 意味を保ちつつ簡潔な英語ログにする（例: `Signaling URL is not set` / `Channel ID is not set` / `RPC message kind is not selected` / `Invalid RPC timeoutMillis format`）

## 完了条件

- `SoraSample.cs` の Debug.Log 系がすべて英語になっている
- `SoraSample.cs` の `//` コメントがすべて日本語になっている
- AGENTS.md の「ログ英語 / コメント日本語」規約に完全準拠している
