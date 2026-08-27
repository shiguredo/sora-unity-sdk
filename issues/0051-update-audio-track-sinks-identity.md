# Sora.cs の audioTrackSinks Dictionary の識別方法を安全にする

- Priority: Medium
- Created: 2026-08-27
- Branch: update/audio-track-sinks-identity
- Polished: {YYYY-MM-DD}

## 目的

`SoraUnitySdkExamples/Assets/SoraUnitySdk/Sora.cs` の `audioTrackSinks` Dictionary が `IAudioTrackSink` 参照をキーに使っているため、ユーザーが `AudioTrack.RemoveSink` に同一データを持つ別インスタンスを渡すと除去されず、ネイティブ側 sink がリークする問題を解消する。

## 現状

`Sora.cs` の `audioTrackSinks` は `Dictionary<IAudioTrackSink, AudioTrackSinkAdapter>` として宣言されており、`AudioTrack.AddSink` / `AudioTrack.RemoveSink` のキーとして `IAudioTrackSink` 参照そのものを利用している。

`Dictionary` の等価比較は参照比較になるため、`AddSink` 時と `RemoveSink` 時で異なるインスタンスを渡した場合、`RemoveSink` は該当エントリを見つけられずネイティブ側 `AudioTrackSinkImpl` が破棄されない。ドキュメントには「同一参照でのみ RemoveSink できる」旨の記述がない。

## 設計方針

以下のいずれかで解決する。

- 設計方針 A: 参照 API を維持しつつ、ドキュメントに「同一 `IAudioTrackSink` インスタンスを AddSink と RemoveSink で渡すこと」を明記する。docstring と CHANGES.md で明示する。
- 設計方針 B: 内部で `IAudioTrackSink` を識別する ID を発行し、`AddSink` は ID を返す、`RemoveSink` は ID で受け取る API に変更する。既存 API と非互換になるので `CHANGE` 扱い。

設計方針 A を先に採用し、B は次期メジャーで検討する。

## 完了条件

- `AudioTrack.AddSink` / `AudioTrack.RemoveSink` の docstring に「同一 `IAudioTrackSink` インスタンスを渡す必要がある」旨が明記されている
- 別インスタンスを渡した場合の挙動（除去されずリークする）が docstring に明示されている
- `CHANGES.md` の `## develop` に該当記述が追加されている
