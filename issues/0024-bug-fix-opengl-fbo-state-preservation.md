# OpenGL キャプチャで FBO 状態の保存/復元と完全性チェックを追加する

- Priority: High
- Created: 2026-08-27
- Branch: feature/fix-opengl-fbo-state-preservation
- Polished: 2026-08-31
- Milestone: 2026.2.0

## 目的

`UnityCameraCapturer::OpenglImpl::Capture` は Unity 側の現在の FBO バインディングを保存せず、`glReadPixels` 後の復元もしない。加えて `glFramebufferTexture2D` の直後に `glCheckFramebufferStatus` による完全性チェックも行っていない。Unity の描画ターゲットが本キャプチャ用 FBO のまま残る恐れがあり、不完全な FBO からの読み出しがドライバ依存の挙動で行われる。

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

`glCheckFramebufferStatus` を省いているため、不完全な FBO からの `glReadPixels` の挙動はドライバ依存になる。GL エラーを返す実装では `GL_ERRCHECK` が `nullptr` を返してフレームを送信しないが、エラーを返さずに読み出しが成功したように振る舞う実装では、正しくない映像が Sora に送信される。なお `buf` は `new uint8_t[width_ * height_ * 4]()` でゼロ初期化されており、`glReadPixels` が失敗しても未初期化データが送信されることはない。

## 設計方針

- Bind 前に現在の FBO を `glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fbo);` で保存する
- 正常系・GL エラーによる早期 return・完全性チェック失敗のすべてのパスで、`glReadPixels` の後に `glBindFramebuffer(GL_FRAMEBUFFER, prev_fbo);` による復元が必ず実行されるようにする
  - 既存の `GL_ERRCHECK` マクロはエラー発生時に即 `return` するため、復元処理が早期 return パスでも実行される実装にする
- 初期化ブロック内で `glFramebufferTexture2D` の直後に `glCheckFramebufferStatus(GL_FRAMEBUFFER)` を実行し、`GL_FRAMEBUFFER_COMPLETE` 以外の場合は次のように処理する
  - `prev_fbo` へ復元してから `fbo_` を破棄する (バインド中の FBO を破棄するとバインディングはデフォルトフレームバッファ 0 に戻るため、先に復元する)
  - `initialized_` は false のまま残し、`nullptr` を返す
- 本 issue は issue 0018 (OpenGL initialized_ フラグ位置修正) の修正が反映済みであることを前提とする
  - 0018 の修正により `initialized_ = true` は成功パスの最後に移動しているため、完全性チェック失敗時は `initialized_` が false のまま残り、次回 Capture で再初期化を試みる
- `glReadPixels` 失敗時の扱いは既存の `GL_ERRCHECK` を維持する
  - GL エラー時は `nullptr` を返してフレームを送信しない
  - `buf` はゼロ初期化済みのため、仮にエラーを検出できなくても未初期化データが送信されることはない

## 完了条件

- OpenGL キャプチャで Unity 側の FBO バインディングが全リターンパスで復元される
- FBO 完全性チェックが初期化時に行われ、不完全な場合は `fbo_` が破棄され、次回 Capture で再初期化が試みられる
- 不完全な FBO に対して `glReadPixels` が実行されず、誤った映像が Sora に送信されない
- Ubuntu / Android 実機で回帰動作確認する
- `CHANGES.md` の `## develop` に `[FIX] OpenGL キャプチャで FBO 状態の保存/復元と完全性チェックを追加する` を追記する
