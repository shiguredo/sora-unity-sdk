# Metal キャプチャで `-fobjc-arc` を有効化してストロング参照で保持する設計に変更する

- Created: 2026-08-29
- Completed: {YYYY-MM-DD}
- Branch: feature/change-arc-metal-capture
- Polished: 2026-09-11

## 目的

macOS / iOS ターゲットの Objective-C++ コードを ARC (Automatic Reference Counting) でビルドし、Metal キャプチャが保持するテクスチャをストロング参照で管理できるようにする。MRC (Manual Retain Count) 前提の手動 `release` を廃し、解放漏れを構造的に防ぐ。

## 現状

`CMakeLists.txt` の macOS (`macos_x86_64` / `macos_arm64`) ターゲットと iOS (`ios`) ターゲットの `target_compile_options` に `-fobjc-arc` の指定がなく、`.mm` ファイルは MRC でビルドされている。

`src/unity_camera_capturer.h` の `UnityCameraCapturer::MetalImpl` は `frame_texture_` を `void*` 型で保持している。`src/unity_camera_capturer_metal.mm` の `MetalImpl::Init` は、Cocoa の命名規則上 retainCount 1 で返る `newTextureWithDescriptor:` メソッドが生成したテクスチャを `void*` にキャストして `frame_texture_` に保存しており、MRC では呼び出し側の明示的な `release` が必要になる。

現時点では `MetalImpl` にデストラクタも明示的な `release` もなく、テクスチャは解放されずにリークしたままである。open の `issues/0010-bug-fix-metal-texture-leak.md` は、デストラクタで明示的な `release` を呼ぶ MRC 前提の対症療法を提案しているが、テクスチャの生成・解放を呼び出し側が手動で管理し続ける限り解放漏れのリスクは常に付きまとう。本 issue はその MRC アプローチを置き換え、ARC 導入で解放漏れを構造的に防ぐ。

## 設計方針

- `CMakeLists.txt` の macOS / iOS ターゲットの `target_compile_options` に `-fobjc-arc` を追加する
- `src/unity_camera_capturer.h` は `unity_camera_capturer.cpp` などの C++ 翻訳単位からもインクルードされ、`UnityCameraCapturer::Init` の `kUnityGfxRendererMetal` 分岐で `MetalImpl` を `new` するため、`MetalImpl` のメンバに `id<MTLTexture>` のような Objective-C 型を導入することはできない（`id` は C++ では未定義となりビルドが失敗する）。ストロング参照で保持する `id<MTLTexture>` は `.mm` 側で管理し、ヘッダの公開型には Objective-C 型を追加しない（例えば `MetalImpl` に PIMPL を導入し、所有権を持つストレージを `unity_camera_capturer_metal.mm` 内で定義する）。`frame_texture_` の型変更に伴い `MetalImpl` にデストラクタの宣言が必要になる場合はヘッダに宣言を追加し、実装は `.mm` 側に置く
- `src/unity_camera_capturer_metal.mm` の `MetalImpl::Init` は `newTextureWithDescriptor:` が返すテクスチャを `.mm` 側のストロング参照へ代入する。ARC 有効化後の `void*` と `id` の相互変換には `__bridge` 系キャストが必要になる点に注意する。ストロング参照へ移す場合は `__bridge_retained`、所有権を持たずに参照する場合は `__bridge` を使う
- `camera_texture_` は Unity 側が所有するテクスチャのため、ストロング参照で保持すると二重解放の原因になる。ヘッダでは `void*` のまま維持し、`unity_camera_capturer_metal.mm` 内で使用する際は `__bridge` キャストで非所有参照を得る（`__unsafe_unretained` で保持する場合は `.mm` 側の型で行う）
- `src/mac_helper/ios_audio_init.mm` の `IosAudioInit` が ARC 下で正しくビルド・動作することを確認する
- MRC 前提で書かれた明示的な `release` 呼び出しが残っている場合は削除する。`issues/0010-bug-fix-metal-texture-leak.md` の MRC 修正が本対応より先に実装された場合も、ARC 下では `release` の直接呼び出しがコンパイルエラーになるため、そのデストラクタと `release` を削除してストロング参照に置き換える

## 完了条件

- macOS / iOS ターゲットで `-fobjc-arc` が有効になり、Metal キャプチャが生成したテクスチャがストロング参照で保持されて明示的な `release` が不要になる
- Instruments 等で `Connect` / `SwitchCamera` を繰り返し実行しても、テクスチャのリークや二重解放が発生しないことを確認する
