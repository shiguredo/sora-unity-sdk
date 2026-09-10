# SoraSample の OnRemoveTrack で GetVideoTrackFromVideoSinkId の例外経路を回避する

- Priority: High
- Created: 2026-08-27
- Branch: fix/sample-on-remove-track-exception
- Polished: {YYYY-MM-DD}
- Milestone: 2026.2.0

## 目的

`SoraSample.cs` の `OnRemoveTrack` ラムダが、削除完了済みの videoSinkId に対して `GetVideoTrackFromVideoSinkId` を呼び、`InvalidOperationException` を投げる経路を修正する。サンプルコードは利用者が真似する対象であり、ここで例外が飛ぶと利用者のプロジェクトも同じ罠に陥る。

## 現状

`SoraUnitySdkExamples/Assets/SoraSample.cs` の `OnRemoveTrack` ラムダは Debug.LogFormat の中で `sora.GetVideoTrackFromVideoSinkId(videoSinkId).Id` を呼び出している。しかし以下の理由で例外が発生する。

- `Sora.cs` の `GetVideoTrackFromVideoSinkId` は該当 videoSinkId が見つからないときに `InvalidOperationException` を投げる
- `OnRemoveTrack` はネイティブ側でトラックが除去された後に呼ばれる可能性が高く、そのタイミングでは videoSinkId が既に無効
- ログ生成のためだけの参照で例外が飛び、ハンドラ全体が中断する

送信側切断→受信側切断の順序が乱れると再現する。リリース検証でも踏まれ得る。

## 設計方針

- `OnRemoveTrack` の Debug.LogFormat から `GetVideoTrackFromVideoSinkId(videoSinkId).Id` の呼び出しを外す
- どうしても Id が必要な場合は事前に OnAddTrack のタイミングで videoSinkId と Id の対応をキャッシュし、キャッシュから取得する
- あるいは `Sora.cs` 側に「例外を投げない Try 版」を追加してサンプルで使う

## 完了条件

- `OnRemoveTrack` 内でトラック削除順序に依存する例外が発生しない
- サンプルシーンで送信側と受信側の切断を任意の順序で行っても例外が飛ばない
- サンプルとして利用者が安全にコピーできる形になっている
- `CHANGES.md` の `## develop` に `[FIX]` を追記する
