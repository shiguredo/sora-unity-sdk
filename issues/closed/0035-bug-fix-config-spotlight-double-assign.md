# Config の Spotlight 二重代入を整理する

- Priority: High
- Created: 2026-08-27
- Branch: fix/config-spotlight-double-assign
- Polished: {YYYY-MM-DD}
- Milestone: 2026.2.0

## 目的

`SoraUnitySdkExamples/Assets/SoraUnitySdk/Sora.cs` の `Config` から proto 用 ConnectConfig への変換で、Spotlight プロパティが二重に代入されている冗長かつ Nullable 意図に反する記述を修正する。Multistream / Simulcast など他の Nullable プロパティと扱いを揃える。

## 現状

`Sora.cs` の `Connect(Config config)` 内で、`cc` に対して次のような処理が並んでいる。

- `if (config.Multistream.HasValue) { cc.SetMultistream(config.Multistream.Value); }`
- `if (config.Spotlight.HasValue) { cc.SetSpotlight(config.Spotlight.Value); }`
- 直後に `cc.spotlight = config.Spotlight.GetValueOrDefault();`

問題点:

- `SetSpotlight` は `has_spotlight` フラグと値の両方を設定するのが期待動作である
- 直後の `cc.spotlight` 生代入は `has_spotlight` は変更せずに値だけ書き換えるため、Nullable の有無情報が乱れる
- 同じ Nullable プロパティである Multistream / Simulcast / SimulcastRequestRid は `HasValue` チェックのみで生代入していない
- 読み手が「なぜ Spotlight だけ生代入が続くのか」を理解できず、意図しない挙動を引き起こす懸念がある

## 設計方針

- `cc.spotlight = config.Spotlight.GetValueOrDefault();` の生代入を削除する
- Nullable プロパティは `HasValue` チェック + `SetXxx` 呼び出しに統一する
- 同じ関数内の他の Nullable プロパティも一貫した書き方になっているか合わせて確認する
- 生代入を残す必要がある場合はコードコメントに理由を書く（残す必要は無いと判断されている）

## 完了条件

- Spotlight の Nullable 扱いが Multistream / Simulcast と同じパターンになっている
- `Config.Spotlight` が null のときに proto 側で `has_spotlight` が正しく false になる
- 動作確認として Spotlight を指定した接続と指定しない接続の双方で意図した proto が生成されている
- `CHANGES.md` の `## develop` に `[FIX]` を追記する
