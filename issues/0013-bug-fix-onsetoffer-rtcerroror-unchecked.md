# OnSetOffer で AddTrack の RTCErrorOr を検査するように修正する

- Priority: Critical
- Created: 2026-08-27
- Branch: fix/onsetoffer-rtcerroror-check
- Polished: {YYYY-MM-DD}
- Milestone: 2026.2.0

## 目的

`Sora::OnSetOffer` が `PeerConnection::AddTrack` の `RTCErrorOr` を検査せずに使っており、AddTrack が失敗した瞬間にサイレントに送信映像が失われる、あるいは `RTC_CHECK` でプロセスが abort する。正式リリース前に必ずエラー処理を追加する。

## 現状

`src/sora.cpp` の `Sora::OnSetOffer` は次のように `AddTrack` の戻り値を扱っている:

```cpp
if (audio_track_ != nullptr) {
  webrtc::RTCErrorOr<webrtc::scoped_refptr<webrtc::RtpSenderInterface>>
      audio_result = signaling_->GetPeerConnection()->AddTrack(audio_track_,
                                                               {stream_id_});
}
if (video_track_ != nullptr) {
  webrtc::RTCErrorOr<webrtc::scoped_refptr<webrtc::RtpSenderInterface>>
      video_result = signaling_->GetPeerConnection()->AddTrack(video_track_,
                                                               {stream_id_});
  video_sender_ = video_result.value();
}
```

`audio_result` は変数を宣言しているだけで `.ok()` チェックも `.value()` 呼び出しも一切ない。AddTrack が失敗しても検出できず、送信サイドが無音のまま接続完了する。

`video_result.value()` は libwebrtc の実装上 `RTC_CHECK` を内包しており、`ok() == false` の場合はプロセスを fatal 終了させる。既存 sender と衝突する、PeerConnection の状態が `Closed` になっているなどのシナリオで発火する。

## 設計方針

- `audio_result` / `video_result` の両方について `if (!result.ok()) { on_disconnect_(INTERNAL_ERROR, result.error().message()); return; }` のようなエラーハンドリングを追加する
- エラーメッセージには失敗した track の種別 (audio / video) を含める
- 早期 return 後に `set_offer_ = true;` が実行されないよう、`OnSetOffer` 全体の制御フローを整理する
- `renderer_->AddTrack(video_track_.get())` 以降の video 系処理も video_sender_ の失敗経路では実行されないようにする

## 完了条件

- `Sora::OnSetOffer` 内のすべての `RTCErrorOr` について `ok()` チェックまたはエラーハンドリングが行われている
- AddTrack 失敗時に `on_disconnect_` が呼ばれ、C# 側にエラー内容が伝わることを確認する
- `RTC_CHECK` によるプロセス abort 経路が排除されている
- 意図的に AddTrack を失敗させるテストシナリオ (例: 既に閉じた PeerConnection に対する AddTrack) でクラッシュしないことを確認する
- `CHANGES.md` の `## develop` に `[FIX] OnSetOffer で AddTrack の RTCErrorOr を検査するようにする` を追記する
