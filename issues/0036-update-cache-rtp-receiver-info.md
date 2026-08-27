# RtpReceiver の StreamIds / Id プロパティ結果をキャッシュする

- Priority: High
- Created: 2026-08-27
- Branch: update/cache-rtp-receiver-info
- Polished: {YYYY-MM-DD}
- Milestone: 2026.2.0

## 目的

`SoraUnitySdkExamples/Assets/SoraUnitySdk/Sora.cs` の `RtpReceiver.StreamIds` / `Id` プロパティが毎回 P/Invoke を 2 回発行し JSON デシリアライズも走らせている実装を、キャッシュに置き換える。receiver 寿命内でこれらの値は不変なため、初回取得のみでよい。

## 現状

`Sora.cs` の `RtpReceiver.StreamIds` と `RtpReceiver.Id` は次の流れで動作している。

- `sora_rtp_receiver_get_info_size` でサイズを取得
- `sora_rtp_receiver_get_info` で JSON をコピー
- `JsonUtility` で `RtpReceiverInfo` に復元
- そこから `stream_ids` / `id` を取り出す

問題点:

- 呼び出しごとに毎回 2 回の P/Invoke と JSON パースが走り、コストが大きい
- `RtpReceiver` 寿命内で `stream_ids` / `id` は不変であるため、キャッシュしない理由が無い
- `OnMediaStreamTrack` や UI 表示など、同じプロパティを短時間に何度も参照するコードでオーバーヘッドが目立つ

## 設計方針

- `RtpReceiver` に `stream_ids` / `id` のキャッシュフィールドを持たせる
- 初回アクセス時に `GetInfo()` を呼び、その結果を保持する
- 2 回目以降はキャッシュを返す
- キャッシュ無効化のシナリオが将来必要になった場合の設計はコードコメントで整理する

## 完了条件

- `RtpReceiver.StreamIds` / `RtpReceiver.Id` の 2 回目以降のアクセスで P/Invoke が発生しない
- 既存の呼び出し側は変更なしで動作する
- `CHANGES.md` の `## develop` に `[UPDATE]` を追記する
