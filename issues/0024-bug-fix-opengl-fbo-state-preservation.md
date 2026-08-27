# OpenGL キャプチャで FBO 状態の保存/復元と完全性チェックを追加する

- Priority: High
- Created: 2026-08-27
- Branch: fix/opengl-fbo-state-preservation
- Polished: {YYYY-MM-DD}
- Milestone: 2026.2.0

## 目的

`UnityCameraCapturer::OpenglImpl::Capture` は Unity 側の現在の FBO バインディングを保存せず、`glReadPixels` 後の復元もしない。加えて `glFramebufferTexture2D` の直後に `glCheckFramebufferStatus` による完全性チェックも行っていない。Unity の描画ターゲットが本キャプチャ用 FBO のまま残る恐れがあり、完全性を欠く FBO からの読み出しで未初期化データが送信される。

## 現状

`src/unity_camera_capturer_opengl.cpp` の `OpenglImpl::Capture` は次のように FBO をバインドする:

```cpp
glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
GL_ERRCHECK("glBindFramebuffer");

std::unique_ptr<uint8_t[]> buf(new uint8_t[width_ * height_ * 4]());

glReadPixels(0, 0, width_, height_, GL_RGBA, GL_UNSIGNED_BYTE, buf.get());
GL_ERRCHECK("glReadPixels");
```

問題点:

- Bind 前に `glGetIntegerv(GL_FRAMEBUFFER_BINDING, ...)` で現在の FBO を保存していない
- 読み出し後に元の FBO に戻す `glBindFramebuffer(GL_FRAMEBUFFER, prev_fbo)` がない
- 初期化時に `glFramebufferTexture2D` の直後で `glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE` を確認していない

Unity 側の描画ターゲットが本キャプチャ用 FBO のまま残ると、直後のフレーム描画が本 FBO に書き込まれ Unity 側のバックバッファが破壊される可能性がある。

`glCheckFramebufferStatus` を省いているため、depth / stencil 無しで作成された texture や、GLES 2 で texture-completeness を満たさないケースで `glReadPixels` が `GL_INVALID_FRAMEBUFFER_OPERATION` を返し、`buf` は未初期化のまま libyuv に渡されて未定義データが Sora に送信される。

## 設計方針

- Bind 前に現在の FBO を `glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fbo);` で保存する
- `glReadPixels` 後に `glBindFramebuffer(GL_FRAMEBUFFER, prev_fbo);` で復元する
- 初期化時に `glCheckFramebufferStatus(GL_FRAMEBUFFER)` を実行し、`GL_FRAMEBUFFER_COMPLETE` 以外であれば初期化失敗として `fbo_` を破棄する
- 前 issue の initialized_ フラグ位置修正と併せて、完全性チェック失敗時のフラグ状態も考慮する
- 未初期化バッファのゼロクリアは `new uint8_t[width_ * height_ * 4]()` (末尾の `()`) で行っているため OK だが、`glReadPixels` が失敗した場合の扱いを明示する

## 完了条件

- OpenGL キャプチャで Unity 側の FBO バインディングが復元される
- FBO 完全性チェックが初期化時に行われ、不完全な場合は fbo_ が破棄される
- `glReadPixels` が失敗した場合の扱いが明示され、未初期化データが Sora に送信されない
- Ubuntu / Android 実機で回帰動作確認する
- `CHANGES.md` の `## develop` に `[FIX] OpenGL キャプチャで FBO 状態の保存/復元と完全性チェックを追加する` を追記する
