# Sora.cs の AMD AMF コメントアウトブロックと VideoTrack TODO を削除する

- Priority: Low
- Created: 2026-08-27
- Branch: fmt/remove-sora-cs-dead-comments
- Polished: 2026-09-11

## 目的

`SoraUnitySdkExamples/Assets/SoraUnitySdk/Sora.cs` に残っている AMD AMF のコメントアウトブロックと、`VideoTrack.AddOrUpdateSink` / `RemoveSink` の TODO ぶら下げを削除する。

## 現状

`Sora.cs` の `GetHardwareAcceleratorPreference` の実装には、AMD AMF の `Merge` 呼び出しの理由コメント (`// AMD AMF は現在非推奨のためコメントアウトする`) と、コメントアウトされた呼び出し (`// preference.Merge(CreateFromImplementation(capability, VideoCodecImplementation.AmdAmf));`) の計 2 行が残っている。CHANGES.md 2025.3.0 に「AMD AMF は非推奨のためコード残置」と記述されているが、コメントアウト状態のまま長期に置かれており、broken windows と化している。「将来また使うかもしれない」という理由でコードを残す判断は、非推奨化の意図とも矛盾する。

`Sora.cs` の `VideoTrack` クラスには `// TODO(melpon): 必要になったら実装する` と `// void AddOrUpdateSink(IVideoSink sink)` / `// void RemoveSink(IVideoSink sink)` のシグネチャだけコメントアウトされたブロック (計 7 行) が、git 履歴上 2025-11-02 に追加されてから約 10 か月放置されている。

## 設計方針

- `GetHardwareAcceleratorPreference` の AMD AMF ブロック (`// AMD AMF は現在非推奨のためコメントアウトする` と `// preference.Merge(CreateFromImplementation(capability, VideoCodecImplementation.AmdAmf));` の 2 行) を削除する
- `VideoTrack` の `// TODO(melpon): 必要になったら実装する` とコメントアウトされた `AddOrUpdateSink` / `RemoveSink` のブロック (計 7 行) を削除する
- 優先順位コメント (`// 優先度的には Intel VPL > AMD AMF > Nvidia Video Codec > Internal となる`) の書き換えは 0054 で対応する。0054 の対象は優先順位記述の書き換えのみであり、上記の AMD AMF ブロックの削除は本 issue が担当する
- 挙動変更は無い

## 完了条件

- `Sora.cs` の `GetHardwareAcceleratorPreference` から AMD AMF の理由コメントとコメントアウトされた `Merge` 呼び出しの計 2 行が消えている
- `Sora.cs` の `VideoTrack` から TODO コメントとコメントアウトされた `AddOrUpdateSink` / `RemoveSink` のブロック (計 7 行) が消えている
- コンパイルが通り、既存の Sora.cs API に回帰が無い
