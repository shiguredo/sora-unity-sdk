# OnSetOffer / DoSwitchCamera で AddTrack の RTCErrorOr を検査するように修正する

- Priority: Critical
- Created: 2026-08-27
- Branch: feature/fix-addtrack-rtcerroror-check
- Polished: 2026-08-29
- Milestone: 2026.2.0

## 目的

`Sora::OnSetOffer` と `Sora::DoSwitchCamera` が `PeerConnection::AddTrack` の `RTCErrorOr` を検査せずに使っており、AddTrack が失敗した瞬間に検出できず送信映像が失われる。特に `video_result.value()` は libwebrtc の実装上 `RTC_DCHECK(ok())` を含み、DCHECK 有効ビルドではプロセスを fatal 終了させ、DCHECK 無効ビルド (リリースビルド) では空の `std::optional` を逆参照して未定義動作になる。正式リリース前に必ずエラー処理を追加する。

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

`video_result.value()` は libwebrtc の実装上 `RTC_DCHECK(ok())` を内包している。DCHECK 有効ビルドでは `ok() == false` の時点でプロセスを fatal 終了させ、DCHECK 無効ビルド (リリースビルド) では空の `std::optional` を逆参照して未定義動作になる。既存 sender と衝突する、PeerConnection の状態が `Closed` になっているなどのシナリオで発火する。

`Sora::DoSwitchCamera` にも同じ未検査パターンが存在する。`video_track_ == nullptr` の場合に `AddTrack` を実行し、`video_result.value()` の結果を検査せずに `video_sender_` へ代入している。

## 設計方針

- `OnSetOffer` の `audio_result` / `video_result` と `DoSwitchCamera` の `video_result` について、すべて `if (!result.ok())` で失敗を検出し、エラーを通知して以降の処理を中断する
- エラー通知は既存の `Sora::OnDisconnect` と同じ方式で `PushEvent` 経由にし、`on_disconnect_` を Unity スレッドで呼び出す (コールバック実行スレッド (signaling スレッド / IO スレッド) から直接呼ばない)

```cpp
if (!result.ok()) {
  PushEvent([this, message = result.error().message()]() {
    if (on_disconnect_) {
      on_disconnect_((int)sora_conf::ErrorCode::INTERNAL_ERROR,
                     std::move(message));
    }
  });
  return;
}
```

- エラーメッセージには失敗した track の種別 (audio / video) を含める
- 早期 return 後に `set_offer_ = true;` が実行されないよう、`OnSetOffer` 全体の制御フローを整理する
- `renderer_->AddTrack(video_track_.get())` 以降の video 系処理も video_sender_ の失敗経路では実行されないようにする

## 完了条件

- `Sora::OnSetOffer` と `Sora::DoSwitchCamera` 内のすべての `RTCErrorOr` について `ok()` チェックまたはエラーハンドリングが行われている
- AddTrack 失敗時に `on_disconnect_` が呼ばれ、C# 側にエラー内容が伝わることを確認する
- 検査なしの `RTCErrorOr::value()` 呼び出しが排除されている
- 意図的に AddTrack を失敗させるテストシナリオ (例: 既に閉じた PeerConnection に対する AddTrack) でクラッシュしないことを確認する
- `CHANGES.md` の `## develop` に `[FIX] OnSetOffer / DoSwitchCamera で AddTrack の RTCErrorOr を検査するようにする` を追記する
