# Android 縦画面配信時のセルフビュー向き不一致を修正する

- Priority: Medium
- Created: 2026-06-08
- Model: Composer 2.5
- Branch: feature/fix-android-portrait-video-orientation
- Polished: 2026-06-08

## 目的

Sora Unity SDK で、Android 端末から縦画面配信した際にセルフビューと受信映像の向きが一致しない問題を修正する。

Unity SDK の `RenderTrackToTexture()` 経路だけ向きがずれる。Unity SDK 本体の描画 API で rotation を扱う。

## 優先度根拠

- Android 縦画面 sendrecv で、SDK 利用者が目にする表示が一貫しない
- 横向き配信では問題ないため High ではない
- 特定の Sora 設定を有効にした環境でのみ再現する

## 現状

Android 端末から縦画面で配信すると、Unity SDK のセルフビュー（`RenderTrackToTexture()`）だけ向きがずれる。横向き配信では一致する。sendrecv 時のセルフビューが主対象で、Unity SDK 同士の Android 送受信でも受信映像の向きがずれる。

Sora 側で video orientation RTP 拡張が有効なときのみ再現し、デフォルト（拡張無効）では再現しない。video orientation RTP 拡張は Sora サーバー側設定であり、Unity SDK から設定する項目は現状ない。

`src/unity_renderer.cpp` の `UnityRenderer::Sink::OnFrame()` は `frame.rotation()` を使わず、`video_frame_buffer` だけを保持している。`RenderTrackToTexture()` は上記 Sink 経由で `Texture2D` を更新するため、rotation メタデータが描画に反映されない。

## 設計方針

- video orientation RTP 拡張が有効な環境で、送信 / 受信 / セルフビューの各段階で `VideoFrame.rotation()` を計測し、原因レイヤーを特定する
- `UnityRenderer` 描画層での修正を優先する。Samples の `RawImage` 回転だけで済ませない
- `UnityRenderer::Sink` が `OnFrame()` で受け取った `VideoFrame.rotation()` を保持し、`TextureUpdateCallback()` で I420 → ABGR 変換時に rotation を反映する（libyuv の rotation 系 API 等）
- `unity_camera_capturer.cpp` の `kVideoRotation_0` 固定が原因と判明した場合は、capturer 層も修正する

## 完了条件

- video orientation RTP 拡張が有効な環境で、Android 縦画面 sendrecv 時にセルフビューと受信映像の向きが一致する
- Sora 側デフォルト（拡張無効）の既存挙動を壊さない
- 横向き配信の既存挙動を壊さない
- `CHANGES.md` の `## develop` に `[FIX] Android 縦画面配信時の映像向き不一致を修正する` を追記する

## 解決方法

1. video orientation RTP 拡張を有効にした Sora 環境で `multi_sendrecv` を使い、`UnityRenderer::Sink::OnFrame()` と `TextureUpdateCallback()` の `rotation` 値を計測して原因レイヤーを特定する
2. `src/unity_renderer.h` / `src/unity_renderer.cpp` で rotation 保持と描画反映を実装する
3. 必要なら `src/unity_camera_capturer.cpp` も修正する
4. Android 実機で縦画面 / 横向き / 拡張無効時の回帰確認を行う
5. `CHANGES.md` に `[FIX]` を追記する
