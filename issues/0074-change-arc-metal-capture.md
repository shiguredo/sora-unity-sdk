# Metal キャプチャで `-fobjc-arc` を有効化してストロング参照で保持する設計に変更する

- Created: 2026-08-29
- Completed: {YYYY-MM-DD}
- Branch: feature/change-arc-metal-capture
- Polished: {YYYY-MM-DD}

## 目的

macOS / iOS ターゲットの Objective-C++ コードを ARC (Automatic Reference Counting) でビルドし、Metal キャプチャが保持するテクスチャをストロング参照で管理できるようにする。MRC (Manual Retain Count) 前提の手動 `release` を廃し、解放漏れを構造的に防ぐ。

## 現状

`CMakeLists.txt` の macOS (`macos_x86_64` / `macos_arm64`) ターゲットと iOS (`ios`) ターゲットの `target_compile_options` に `-fobjc-arc` の指定がなく、`.mm` ファイルは MRC でビルドされている。

`src/unity_camera_capturer.h` の `UnityCameraCapturer::MetalImpl` は `frame_texture_` を `void*` 型で保持している。`src/unity_camera_capturer_metal.mm` の `MetalImpl::Init` は、Cocoa の命名規則上 retainCount 1 で返る `newTextureWithDescriptor:` メソッドが生成したテクスチャを `void*` にキャストして `frame_texture_` に保存しており、MRC では呼び出し側の明示的な `release` が必要になる。

現在の対処方針は、デストラクタで `frame_texture_` に明示的な `release` を呼び出すという MRC 前提の対症療法である。テクスチャの生成・解放を呼び出し側が手動で管理し続ける限り、解放漏れのリスクは常に付きまとう。

## 設計方針

- `CMakeLists.txt` の macOS / iOS ターゲットの `target_compile_options` に `-fobjc-arc` を追加する
- `src/unity_camera_capturer.h` の `MetalImpl` の `frame_texture_` を `id<MTLTexture>` のストロング参照に変更し、明示的な `release` を不要にする
- `src/unity_camera_capturer_metal.mm` の `MetalImpl::Init` で `newTextureWithDescriptor:` が返すテクスチャをストロング参照で保持する。ARC 有効化後は `void*` と `id` の相互変換に `__bridge` キャストが必要になる点に注意する
- `camera_texture_` は Unity 側が所有するテクスチャのため、ストロング参照で保持すると二重解放の原因になる。`__unsafe_unretained` 等で保持する
- `src/mac_helper/ios_audio_init.mm` の `IosAudioInit` が ARC 下で正しくビルド・動作することを確認する
- MRC 前提で書かれた明示的な `release` 呼び出しが残っている場合は削除する

## 完了条件

- macOS / iOS ターゲットで `-fobjc-arc` が有効になり、`MetalImpl` の `frame_texture_` がストロング参照で保持されて明示的な `release` が不要になる
- Instruments 等で `Connect` / `SwitchCamera` を繰り返し実行しても、テクスチャのリークや二重解放が発生しないことを確認する
