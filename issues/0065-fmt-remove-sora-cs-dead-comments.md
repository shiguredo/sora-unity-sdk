# Sora.cs の AMD AMF コメントアウトブロックと VideoTrack TODO を削除する

- Priority: Low
- Created: 2026-08-27
- Branch: fmt/remove-sora-cs-dead-comments
- Polished: {YYYY-MM-DD}

## 目的

`SoraUnitySdkExamples/Assets/SoraUnitySdk/Sora.cs` に残っている AMD AMF のコメントアウトブロックと、`VideoTrack.AddOrUpdateSink` / `RemoveSink` の TODO ぶら下げを削除する。

## 現状

`Sora.cs` の `GetHardwareAcceleratorPreference` の実装には、AMD AMF の `Merge` 呼び出しが一連のコメントアウトブロックとして残っている。CHANGES.md 2025.3.0 に「AMD AMF は非推奨のためコード残置」と記述されているが、コメントアウト状態のまま長期に置かれており、broken windows と化している。「将来また使うかもしれない」という理由でコードを残す判断は、非推奨化の意図とも矛盾する。

`Sora.cs` の `VideoTrack` クラスには `// TODO(melpon): 必要になったら実装する` と `// void AddOrUpdateSink(IVideoSink sink)` / `// void RemoveSink(IVideoSink sink)` のシグネチャだけコメントアウトされたブロックが 3 年以上放置されている。

## 設計方針

- AMD AMF の `Merge` コメントアウトブロックを削除する
- `VideoTrack.AddOrUpdateSink` / `RemoveSink` の TODO ブロックを削除する
- `GetHardwareAcceleratorPreference` のコメント修正は別 issue で対応する
- 挙動変更は無い

## 完了条件

- `Sora.cs` から AMD AMF の `Merge` コメントアウトブロックが消えている
- `VideoTrack` の TODO ぶら下げが消えている
- コンパイルが通り、既存の Sora.cs API に回帰が無い
