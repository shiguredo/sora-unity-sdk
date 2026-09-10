# Nullable enable を他の C# ファイルに広げる

- Priority: High
- Created: 2026-08-27
- Branch: update/expand-nullable-enable
- Polished: 2026-09-10
- Milestone: 2026.2.0

## 目的

CHANGES.md 2025.3.0 で `Sora.cs` に `#nullable enable` を導入したが、他の C# ファイルには広がっていない。sora-unity-sdk 側で提供する C# コード全体で Nullable 対応を揃え、Nullable 対応の宣言と実態を一致させる。

## 現状

以下の C# ファイルには `#nullable enable` が付いていない。

- `SoraUnitySdkExamples/Assets/SoraSample.cs`
- `SoraUnitySdkExamples/Assets/SoraUnitySdk/Editor/SoraUnitySdkPostProcessor.cs`

`Sora.cs` だけが Nullable 対応になっており、他ファイルが取り残されている。Sora クラス側では nullable 参照を返す API を持っているが、呼び出し側の SoraSample.cs が Nullable コンテキストで書かれていないため、nullable 情報が生かせない状態にある。

## 設計方針

- 変更対象は `SoraSample.cs` と `SoraUnitySdkPostProcessor.cs` の 2 ファイルのみとし、先頭に `#nullable enable` を追加する
- 既に `#nullable enable` が入っている `Sora.cs` は変更しない（`AudioTrackSinkAdapter` など Sora.cs 内の既存実装は本 issue の対象外）
- コンパイラ警告が出る箇所は nullable 型への見直しで解消する（例: `SoraSample.cs` の `DumpDeviceInfo` への null 許容戻り値の受け渡し、`ConnectionInfo.audioTrackSink` の未初期化フィールド）
- null に対する扱いを型で表現できない箇所（例: `SoraSample.cs` の `OnAddTrack` で `SenderAudioTrackSink` を `AudioTrackSink` にキャストする箇所、Unity インスペクター用のフィールド）は、null チェックの追加または null にならない理由をコードコメントで明記する
- サンプルの利用者が Nullable 対応の書き方の参考にできる状態にする

## 完了条件

- `SoraSample.cs` と `SoraUnitySdkPostProcessor.cs` に `#nullable enable` が入っている
- Nullable 関連の警告がすべて解消されている、または意図的な抑制が明記されている
- 既存の Sora.cs との Nullable 契約が整合している
- `CHANGES.md` の `## develop` に `[UPDATE]` を追記する
