# D3D11 キャプチャで ID3D11Texture2D を確実にリークする問題を修正する

- Priority: Critical
- Created: 2026-08-27
- Branch: fix/d3d11-texture-leak
- Polished: {YYYY-MM-DD}
- Milestone: 2026.2.0

## 目的

Windows で `UnityCameraCapturer` が使う `D3D11Impl` が `ID3D11Texture2D` をリークする問題を修正する。`Connect` や `SwitchCamera` を繰り返すたびにテクスチャリソースが漏れ、長時間動作で GPU リソースが枯渇する。正式リリース前に必ず対応が必要。

## 現状

`src/unity_camera_capturer.h` の `UnityCameraCapturer::D3D11Impl` にはデストラクタの宣言・実装がない:

```cpp
class D3D11Impl : public Impl {
  UnityContext* context_ = nullptr;
  void* camera_texture_ = nullptr;
  void* frame_texture_ = nullptr;
  int width_ = 0;
  int height_ = 0;

 public:
  bool Init(...) override;
  webrtc::scoped_refptr<webrtc::I420Buffer> Capture() override;
};
```

一方 `src/unity_camera_capturer_d3d11.cpp` の `D3D11Impl::Init` では `device->CreateTexture2D(&desc, NULL, &texture)` により参照カウント 1 の `ID3D11Texture2D*` を受け取り `frame_texture_ = texture;` として `void*` に保存している。

同一ヘッダ内の他プラットフォーム実装 `D3D12Impl` / `VulkanImpl` / `OpenglImpl` にはいずれもデストラクタが宣言されており、対応するリソースの解放処理を持っている。D3D11 だけデストラクタが抜けている。

`UnityCameraCapturer` は `~Sora` で `nullptr` 代入されて `capturer_` の scoped_refptr 経由で破棄されるが、`D3D11Impl` のデストラクタがないため `frame_texture_` に保持している ID3D11Texture2D が `Release()` されない。

## 設計方針

- `src/unity_camera_capturer.h` の `D3D11Impl` にデストラクタを宣言する
- `src/unity_camera_capturer_d3d11.cpp` に以下のようなデストラクタを実装する

```cpp
UnityCameraCapturer::D3D11Impl::~D3D11Impl() {
  if (frame_texture_ != nullptr) {
    static_cast<ID3D11Texture2D*>(frame_texture_)->Release();
    frame_texture_ = nullptr;
  }
}
```

- 他プラットフォーム実装 (`D3D12Impl` / `VulkanImpl` / `OpenglImpl`) と対称な構造になるようにヘッダの宣言順序を揃える

## 完了条件

- `D3D11Impl` のデストラクタが宣言・実装されている
- `Connect` / `SwitchCamera` を 100 回程度繰り返すソークで、`ID3D11Texture2D` の生成数と解放数が一致することを Windows で確認する
- 他プラットフォーム実装との対称性が保たれている
- `CHANGES.md` の `## develop` に `[FIX] Windows D3D11 キャプチャの ID3D11Texture2D リークを修正する` を追記する
