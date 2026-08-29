# Metal キャプチャで MTLTexture をリークする問題を修正する

- Priority: Critical
- Created: 2026-08-27
- Branch: feature/fix-metal-texture-leak
- Polished: 2026-08-29
- Milestone: 2026.2.0

## 目的

macOS / iOS で `UnityCameraCapturer` が使う `MetalImpl` が `MTLTexture` をリークする問題を修正する。`CMakeLists.txt` に `-fobjc-arc` の指定がなく MRC (Manual Retain Count) 環境でビルドされているため、明示的な release が必要にも関わらず解放が行われていない。

## 現状

`src/unity_camera_capturer.h` の `UnityCameraCapturer::MetalImpl` にはデストラクタの宣言・実装がない。同じヘッダの `D3D12Impl` / `VulkanImpl` / `OpenglImpl` にはデストラクタが宣言されているのに、`MetalImpl` だけ抜けている。

`src/unity_camera_capturer_metal.mm` の `MetalImpl::Init` では `newTextureWithDescriptor:` メソッドで `id<MTLTexture>` を生成し、`void*` にキャストして `frame_texture_ = tex2;` として保存している:

```objc
auto tex2 = [device newTextureWithDescriptor:descriptor];
frame_texture_ = tex2;
```

Cocoa の命名規則により `new` プレフィックスで始まるメソッドは retainCount 1 で返る (呼び出し側が release する責任を持つ)。`CMakeLists.txt` に `-fobjc-arc` の指定はなく、macOS / iOS ターゲット両方とも MRC でビルドされているため、`release` を明示的に呼ばない限り MTLTexture は解放されない。

`MetalImpl` のデストラクタが存在しないため、`Sora` を破棄するたびに MTLTexture がリークする。

## 設計方針

- `src/unity_camera_capturer.h` の `MetalImpl` にデストラクタを宣言する
- `src/unity_camera_capturer_metal.mm` に以下のようなデストラクタを実装する

```objc
UnityCameraCapturer::MetalImpl::~MetalImpl() {
  if (frame_texture_ != nullptr) {
    [(id<MTLTexture>)frame_texture_ release];
    frame_texture_ = nullptr;
  }
}
```

- 併せて `MetalImpl` のメンバを `D3D11Impl` / `D3D12Impl` と同様にデフォルト初期化する (`context_ = nullptr` / `camera_texture_ = nullptr` / `frame_texture_ = nullptr` / `width_ = 0` / `height_ = 0`)

## 完了条件

- `MetalImpl` のデストラクタが宣言・実装されている
- `MetalImpl` のメンバがデフォルト初期化され、`Init` 未実行のまま破棄されてもデストラクタが安全に動作する
- macOS と iOS の両方で `Connect` / `SwitchCamera` を繰り返し実行しても MTLTexture の生成数と解放数が一致することを Instruments 等で確認する
- `CHANGES.md` の `## develop` に `[FIX] macOS / iOS Metal キャプチャの MTLTexture リークを修正する` を追記する
