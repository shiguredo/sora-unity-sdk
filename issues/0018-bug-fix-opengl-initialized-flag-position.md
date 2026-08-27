# OpenGL キャプチャの initialized_ フラグ設定位置を修正する

- Priority: Critical
- Created: 2026-08-27
- Branch: fix/opengl-initialized-flag-position
- Polished: {YYYY-MM-DD}
- Milestone: 2026.2.0

## 目的

`UnityCameraCapturer::OpenglImpl::Capture` は `initialized_ = true` を初期化処理の先頭でセットしてから GL 呼び出しを実行するため、`glGenFramebuffers` などが失敗すると `fbo_ = 0` (デフォルトフレームバッファ) のまま `initialized_` が true に残る。次回 Capture 以降は「Unity スクリーン全体をキャプチャして Sora に配信する」経路になり、情報漏えいの直接原因になる。単なる映像化けで済まない性質のため、正式リリース前に必ず修正する。

## 現状

`src/unity_camera_capturer_opengl.cpp` の `OpenglImpl::Capture` は次のように書かれている:

```cpp
webrtc::scoped_refptr<webrtc::I420Buffer>
UnityCameraCapturer::OpenglImpl::Capture() {
  if (!initialized_) {
    initialized_ = true;

    glGenFramebuffers(1, &fbo_);
    GL_ERRCHECK("glGenFramebuffers");

    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    GL_ERRCHECK("glBindFramebuffer");

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           (GLuint)(intptr_t)camera_texture_, 0);
    GL_ERRCHECK("glFramebufferTexture2D");
  }

  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
  ...
  glReadPixels(0, 0, width_, height_, GL_RGBA, GL_UNSIGNED_BYTE, buf.get());
```

`GL_ERRCHECK` マクロは GL エラー発生時に `nullptr` を return するが、その時点で `initialized_` は既に `true` にセット済み。次回 Capture では `if (!initialized_)` が false と評価されて初期化スキップ、`fbo_ = 0` のまま `glBindFramebuffer(GL_FRAMEBUFFER, 0)` を実行することになる。

OpenGL の仕様では `glBindFramebuffer(GL_FRAMEBUFFER, 0)` は「デフォルトフレームバッファ」(=画面) をバインドするため、続く `glReadPixels` が Unity のスクリーンバッファから直接読み出す。結果として意図せず Unity 全体の画面が Sora に配信されることになる。

## 設計方針

- `initialized_ = true` の代入位置を成功パスの最後尾に移動する
  - `glGenFramebuffers` / `glBindFramebuffer` / `glFramebufferTexture2D` がすべて成功してから `initialized_ = true` にする
- 失敗時は `fbo_ = 0` のまま次回 Capture で再初期化を試みるか、あるいは `fbo_` が 0 の間は Capture 全体をスキップする防御を追加する
- `fbo_ = 0` を検知した場合 `glBindFramebuffer(GL_FRAMEBUFFER, 0)` を呼ぶのではなく即 nullptr を返すガードを追加する (デフォルト FB へのフォールバックを絶対に許さない)

## 完了条件

- OpenGL キャプチャの初期化が失敗しても、次回以降に Unity スクリーン全体がキャプチャされない
- `fbo_ == 0` のまま `glBindFramebuffer(GL_FRAMEBUFFER, fbo_)` を実行する経路が存在しない
- Ubuntu / Android の実機で意図的に GL エラーを誘発するテストシナリオを実行しても情報漏えいが発生しない
- `CHANGES.md` の `## develop` に `[FIX] OpenGL キャプチャの初期化失敗時にデフォルト FB がバインドされないよう修正する` を追記する
