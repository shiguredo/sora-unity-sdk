# RtpReceiver の StreamIds / Id プロパティ結果をキャッシュする

- Priority: High
- Created: 2026-08-27
- Branch: update/cache-rtp-receiver-info
- Polished: 2026-09-10
- Milestone: 2026.2.0

## 目的

`SoraUnitySdkExamples/Assets/SoraUnitySdk/Sora.cs` の `RtpReceiver.StreamIds` / `Id` プロパティがアクセスのたびに P/Invoke を 2 回発行し JSON デシリアライズも走らせている実装を、キャッシュに置き換える。Sora の接続フローでは受信ストリームはトラック追加後に変化せず（再ネゴシエーションの仕組みはこの SDK にはない）、これらの値はトラック追加後は不変なため、初回取得のみでよい。

## 現状

`Sora.cs` の `RtpReceiver.StreamIds` と `RtpReceiver.Id` は次の流れで動作している。

- `sora_rtp_receiver_get_info_size` でサイズを取得
- `sora_rtp_receiver_get_info` で JSON をコピー
- `Jsonif.Json.FromJson` で `SoraConf.Internal.RtpReceiverInfo` に復元（Unity の `JsonUtility` ではなく `Jsonif` を使用している）
- そこから `stream_ids` / `id` を取り出す

問題点:

- 呼び出しごとに毎回 2 回の P/Invoke と JSON パースが走り、コストが大きい
- 受信ストリームはトラック追加後に変化せず、ネイティブ側も接続 ID をトラック追加時点で固定して保持している（`src/sora.cpp` の `Sora::OnTrack`）ため、キャッシュしない理由が無い
- SDK 利用者側のコードで、同じプロパティを短時間に何度も参照する場合にオーバーヘッドが大きくなる（このリポジトリのサンプルには現在 `StreamIds` / `Id` の利用は無い）

## 設計方針

- `RtpReceiver` に `GetInfo()` の結果（`RtpReceiverInfo`）を保持するキャッシュフィールドを持たせる
- 初回アクセス時に `GetInfo()` を呼び、その結果を保持する
- 2 回目以降はキャッシュを返す
- キャッシュはマネージド `RtpReceiver` インスタンス単位で有効である。`RtpTransceiver.Receiver` はアクセスのたびに新しい `RtpReceiver` を生成する（`Sora.cs` の `RtpTransceiver.Receiver`）ため、`transceiver.Receiver` を参照のたびに呼ぶコードではキャッシュは効かない。同じインスタンスを使い回す場合にのみ効果があることを前提にする
- `StreamIds` はキャッシュから毎回新しい配列を作って返す（呼び出し側が配列を変更してもキャッシュを壊さない）
- キャッシュ無効化が必要になるのは受信ストリームが変わる場合（将来、再ネゴシエーションに対応した場合など）であり、その設計はコードコメントで整理する

## 完了条件

- 同じ `RtpReceiver` インスタンスへの 2 回目以降の `StreamIds` / `Id` アクセスで P/Invoke が発生しない
- 既存の呼び出し側は変更なしで動作する
- `CHANGES.md` の `## develop` の `### misc` に `[UPDATE]` を追記する
