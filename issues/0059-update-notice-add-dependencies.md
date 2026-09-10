# NOTICE.md に protobuf / protoc-gen-jsonif と libwebrtc 依存の帰属表示を追加する

- Priority: Medium
- Created: 2026-08-27
- Branch: update/notice-add-dependencies
- Polished: 2026-09-10

## 目的

リポジトリ直下の `NOTICE.md` に、protoc と protoc-gen-jsonif から生成され SDK パッケージ（`SoraUnitySdk.zip`）に含まれて再配布される生成コードの帰属表示（protobuf は BSD-3-Clause、protoc-gen-jsonif は Apache-2.0）を追加し、libwebrtc に静的リンクされて再配布される主要な依存ライブラリの列挙も整理する。

## 現状

リポジトリ直下の `NOTICE.md` は Sora C++ SDK / Boost / libwebrtc のライセンス文言のみを収録している。

一方、ビルド時には `protoc` と `protoc-gen-jsonif` を利用して以下の生成コードが `SoraUnitySdkExamples/Assets/SoraUnitySdk/Generated/` に出力される（`CMakeLists.txt` の `sora_conf.json.h` ターゲット。`SoraUnitySdkExamples/.gitignore` によりコミット対象外のため、生成物はリポジトリには含まれない）。

- `SoraUnitySdkExamples/Assets/SoraUnitySdk/Generated/Jsonif.cs`
- `SoraUnitySdkExamples/Assets/SoraUnitySdk/Generated/SoraConf.cs`
- `SoraUnitySdkExamples/Assets/SoraUnitySdk/Generated/SoraConfInternal.cs`

これらは `run.py package`（`_package()`）で `_package/SoraUnitySdk/SoraUnitySdk/` にコピーされ、`SoraUnitySdk.zip` として再配布される。生成コードには protobuf の runtime 依存はないが、生成物の由来として protoc-gen-jsonif（Apache-2.0）と protobuf（BSD-3-Clause）の帰属表示があると誤解が減る。

さらに libwebrtc（`DEPS` の `WEBRTC_BUILD_VERSION` で指定される prebuilt バイナリ）には boringssl / libyuv / opus 等の third_party ライブラリが静的リンクされているが、`NOTICE.md` には触れられていない。なお、openh264 はこの libwebrtc に含まれていない（webrtc-build の README で、提供するビルド済みバイナリには H.264 と H.265 のコーデックが含まれないことが明記されている）。Sora C++ SDK も OpenH264 を動的ロード（`dlopen` / `LoadLibrary`）で使用するため、SDK パッケージに同梱されない。

## 設計方針

- リポジトリ直下の `NOTICE.md` を編集する（`run.py package` がこのファイルをパッケージへコピーする）
- `NOTICE.md` に protobuf（`DEPS` の `PROTOBUF_VERSION`、BSD-3-Clause）の帰属表示を追加する
- `NOTICE.md` に protoc-gen-jsonif（`DEPS` の `PROTOC_GEN_JSONIF_VERSION`、Apache-2.0）の帰属表示を追加する
- libwebrtc に静的リンクされ再配布される主要依存の帰属表示を追加する（boringssl / libyuv / opus を対象とする。根拠は libwebrtc の `include/third_party/boringssl` / `include/third_party/libyuv` と、libwebrtc のデフォルトビルドで opus が含まれること）
- 各依存のライセンス条項全文を書くと肥大化するため、依存の一覧とライセンス種別（Apache-2.0 / BSD-3-Clause / BSD-2-Clause 等）を明記し、詳細は上流の LICENSE を参照する記述にとどめる

## 完了条件

- リポジトリ直下の `NOTICE.md` に protobuf / protoc-gen-jsonif の帰属表示が追加されている
- リポジトリ直下の `NOTICE.md` に libwebrtc 経由の再配布依存（boringssl / libyuv / opus）の一覧が追記されている
- Sora C++ SDK / Boost / libwebrtc の既存記述は温存されている
- `CHANGES.md` の `## develop` に `[UPDATE] NOTICE.md に protobuf / protoc-gen-jsonif と libwebrtc 依存の帰属表示を追加する` を追記する
