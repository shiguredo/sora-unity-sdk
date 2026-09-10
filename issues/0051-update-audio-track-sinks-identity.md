# AudioTrack.AddSink / RemoveSink に同一 IAudioTrackSink インスタンスの指定を明文化する

- Priority: Medium
- Created: 2026-08-27
- Branch: feature/update-audio-track-sinks-identity
- Polished: 2026-09-10

## 目的

`SoraUnitySdkExamples/Assets/SoraUnitySdk/Sora.cs` の `AudioTrack.AddSink` / `AudioTrack.RemoveSink` は、`audioTrackSinks` Dictionary のキーに `IAudioTrackSink` インスタンスそのものを使う。ユーザーが `AddSink` 時と異なるインスタンス（同一データを持つ別インスタンスでも）を `RemoveSink` に渡すと該当エントリが見つからず除去されないため、ネイティブ側 sink が `Sora.Dispose()` まで残る。この誤用を防ぐため、両メソッドに同一の `IAudioTrackSink` インスタンスを渡す必要があることを docstring で明文化する。

## 現状

`Sora.cs` の `audioTrackSinks` は `Dictionary<IAudioTrackSink, AudioTrackSinkAdapter>` として宣言されており、`AudioTrack.AddSink` / `AudioTrack.RemoveSink` のキーとして `IAudioTrackSink` 参照そのものを利用している。

`Dictionary<,>` のキー比較は既定では `EqualityComparer<T>.Default` によるため、`IAudioTrackSink` の実装が `Equals` / `GetHashCode` をオーバーライドしない限り参照比較になる。そのため `AddSink` 時と `RemoveSink` 時で異なるインスタンスを渡した場合、`RemoveSink` は該当エントリを見つけられず、ネイティブ側 `AudioTrackSinkImpl` は破棄されない。この場合に破棄されるのは、`Sora.Dispose()` が `audioTrackSinks` の全エントリを破棄するときだけである。`AudioTrack.AddSink` / `AudioTrack.RemoveSink` には現在 docstring がなく、ドキュメントには「同一参照でのみ RemoveSink できる」旨の記述がない。

## 設計方針

以下のいずれかで解決する。

- 設計方針 A: 参照 API を維持しつつ、`AudioTrack.AddSink` / `AudioTrack.RemoveSink` の docstring に「`AddSink` と `RemoveSink` には同一の `IAudioTrackSink` インスタンスを渡すこと」を明記する。`CHANGES.md` にも補足を追記する。
- 設計方針 B: 内部で `IAudioTrackSink` を識別する ID を発行し、`AddSink` は ID を返す、`RemoveSink` は ID で受け取る API に変更する。既存 API と非互換になるので `CHANGE` 扱い。

A は誤用の防止（利用者への明示）であり、誤った呼び出し自体を構造的に防ぐものではない。構造的な解決は B だが、まず A を採用して制約を明文化し、B は次期メジャーで検討する。

## 完了条件

- `AudioTrack.AddSink` / `AudioTrack.RemoveSink` の docstring に、次の 2 点が明記されている
  - 同一の `IAudioTrackSink` インスタンスを `AddSink` と `RemoveSink` で渡す必要があること
  - 別インスタンスを渡した場合は除去されず、`Sora.Dispose()` までネイティブ側 sink が解放されないこと
- `CHANGES.md` の `## develop` に `[UPDATE]` で該当記述が追加されている
