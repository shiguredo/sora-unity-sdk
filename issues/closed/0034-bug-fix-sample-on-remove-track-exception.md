# SoraSample の OnRemoveTrack で GetVideoTrackFromVideoSinkId の例外経路を回避する

- Priority: High
- Created: 2026-08-27
- Completed: 2026-09-10
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

## 解決方法

コード変更は行わず closed にした。報告されたバグ（`SoraSample.cs` の `OnRemoveTrack` 内で `sora.GetVideoTrackFromVideoSinkId(videoSinkId).Id` を呼ぶと `InvalidOperationException` が投げられる経路）が、現行実装のソース照合では成立しないため。

現行実装（`develop`）での照合結果:

- `SoraSample.cs` の受信側（非 Sendonly）`OnRemoveTrack` ラムダが `Debug.LogFormat` 内で `sora.GetVideoTrackFromVideoSinkId(videoSinkId).Id` を呼び出している点は記述どおり。ただし送信側（Sendonly）`OnRemoveTrack` には当該呼び出しは存在しない
- `Sora.cs` の `GetVideoTrackFromVideoSinkId` がネイティブ側の戻り値 `IntPtr.Zero` に対して `InvalidOperationException` を投げる点も記述どおり（`Sora.cs` の `GetVideoTrackFromVideoSinkId`）
- しかし `IntPtr.Zero` に到達する経路が存在しない。`src/sora.cpp` の `Sora::OnRemoveTrack`（`SoraSignalingObserver` の実装）は PushEvent のラムダ内で次の順に処理する
  1. `renderer_->GetVideoSinkId(video_track)` で `videoSinkId` を取得する
  2. `on_remove_track_` を呼び出す（ここで C# の `OnRemoveTrack` コールバックが動く）
  3. 最後に `renderer_->RemoveTrack(video_track)` を呼ぶ
  つまり C# コールバックは UnityRenderer からのトラック除去より前、かつ直前に同じ `renderer_` から取得した `videoSinkId` を渡されて呼ばれる。`UnityRenderer::GetVideoTrackFromVideoSinkId` は同じ `sinks_` コンテナを `sinkId` で検索する（`src/unity_renderer.cpp` の `GetVideoTrackFromVideoSinkId`）ため、コールバック実行時点の検索は必ず成功し、`InvalidOperationException` は投げられない
- `UnityRenderer::sinks_` を変更する箇所はリポジトリ内に次のみ。いずれも C# コールバックの実行中とは排他になるか、コールバックに到達しない
  - `UnityRenderer::AddTrack`（`Sora::OnTrack` / `Sora::OnSetOffer` のイベント内）
  - `UnityRenderer::RemoveTrack`（`Sora::OnRemoveTrack` のイベント内で、C# コールバックの後）
  - `UnityRenderer::ReplaceTrack`（送信側の `Sora::SwitchCamera` のみ。受信側のサンプル経路では使われない）
  - `renderer_.reset()`（`Sora::OnDisconnect` と `Sora` オブジェクトのデストラクタ）
- issue が想定する「ネイティブ側でトラックが除去された後に呼ばれる」タイミングは存在しない。`renderer_` が破棄済みのケースでは、現行実装では `Sora::OnRemoveTrack` のラムダ内 `renderer_->GetVideoSinkId` が null のまま `operator->` して SEGV する（C# の例外にはならない）。この SEGV 経路は「OnDisconnect の renderer_.reset() を Unity スレッドに寄せる」issue（0030）の対象であり、0030 の設計では `Sora::GetVideoSinkIdFromVideoTrack` が null 時に 0 を返し、`Sora::OnRemoveTrack` のラムダも `renderer_ == nullptr` で早期 return するため、`on_remove_track_`（C# コールバック）は呼ばれない
- 「送信側切断→受信側切断の順序が乱れると再現する」についても、現行実装で問題が起きるとすれば C# の `InvalidOperationException` ではなく前項のネイティブクラッシュ（SEGV）であり、issue の記述（例外）とは調子がずれる。その経路も 0030 で対処される

以上より、報告された `InvalidOperationException` は現行実装では発生せず、0030 適用後も C# サンプルの `OnRemoveTrack` コールバックは `renderer_` が生存し該当トラックが登録されているときだけ呼ばれる。バグ修正 issue として成立しないため closed にする。

備考: polish-issue のレビューで本件が処理不能指摘（報告バグの不存在、実ファイルのソース照合で確定）となったため、`Polished:` は更新していない。
