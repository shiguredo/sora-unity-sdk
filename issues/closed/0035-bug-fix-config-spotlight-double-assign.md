# Config の Spotlight 二重代入を整理する

- Priority: High
- Created: 2026-08-27
- Completed: 2026-09-10
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

## 解決方法

コード変更は行わず closed にした。報告されたバグ（`cc.spotlight` の生代入によって `has_spotlight` の有無情報が乱れ、意図しない挙動を引き起こす）が、現行実装のソース照合では成立しないため。

現行実装（`develop`）での照合結果:

- `Sora.cs` の `Sora.Connect(Sora.Config)` の CC 変換部に `if (config.Spotlight.HasValue) { cc.SetSpotlight(config.Spotlight.Value); }` と、直後の `cc.spotlight = config.Spotlight.GetValueOrDefault();` が並んでいる点は記述どおり
- `proto/sora_conf_internal.proto` の `optional bool spotlight = 10;` は、ビルドで使用する protoc-gen-jsonif-unity 0.13.0（DEPS の `PROTOC_GEN_JSONIF_VERSION`）と protoc 25.9（DEPS の `PROTOBUF_VERSION`）により、`SoraConfInternal.cs` の `ConnectConfig` に次を生成する
  - `public bool spotlight;`（生フィールド）
  - `SetSpotlight(bool)` は `_spotlight_case = SpotlightCase.kSpotlight;` と値の両方を設定する
  - `HasSpotlight()` は `_spotlight_case == SpotlightCase.kSpotlight` で判定する
- つまり `SetSpotlight` が設定する「has 相当」は `spotlight` の値ではなく `_spotlight_case`（proto3 optional を oneof 相当として扱ったケース enum）である
- 生代入 `cc.spotlight = config.Spotlight.GetValueOrDefault();` は `_spotlight_case` を一切変更しない。値の方は
  - `config.Spotlight.HasValue` が true のとき、`GetValueOrDefault()` は `SetSpotlight` に渡した `config.Spotlight.Value` と同一の値を返す
  - false のとき、`SetSpotlight` は呼ばれず `_spotlight_case` は `NOT_SET` のままで、`GetValueOrDefault()` は `false`（デフォルト値）を返すが `_spotlight_case` は変更しない
  - したがって `_spotlight_case` と値の整合性は常に保たれ、「Nullable の有無情報が乱れる」ことはない
- Unity から C++ へは `sora_connect(p, Jsonif.Json.ToJson(cc))` で JSON として渡され、`src/sora.cpp` の `Sora::DoConnect` は `if (cc.has_spotlight()) { config.spotlight = cc.spotlight; }` で適用する。`has_spotlight()` も生成コードの `_spotlight_case` 判定そのものであり、生代入の影響を受けない
- 生代入を削除しても生成される JSON（`_spotlight_case` と `spotlight` の値）は全く変化しないため、二重代入は冗長なだけ（無害）である。issue の完了条件「`Config.Spotlight` が null のときに `has_spotlight` が false になる」「Spotlight 指定の有無で意図した proto が生成される」は現行実装で既に満たされている
- Multistream / Simulcast / SimulcastRequestRid が `HasValue` チェック + `SetXxx` のみである点も記述どおりだが、これは生代入がバグである証明ではなく、Spotlight だけ古い記述が残っていたという冗長コードの話である

以上より、報告された「Nullable の有無情報が乱れる」は現行実装では発生しない。バグ修正 issue として成立しないため closed にする。冗長行の削除自体は挙動を変えない整理であり、必要ならバグではなくリファクタリング（fmt / refactor カテゴリ）の issue として別途扱うべき内容である。

備考: polish-issue のレビューで本件が処理不能指摘（報告バグの不存在、実ファイルのソース照合で確定）となったため、`Polished:` は更新していない。
