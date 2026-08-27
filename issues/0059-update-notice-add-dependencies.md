# NOTICE.md に protobuf と protoc-gen-jsonif の帰属表示を追加する

- Priority: Medium
- Created: 2026-08-27
- Branch: update/notice-add-dependencies
- Polished: {YYYY-MM-DD}

## 目的

`NOTICE.md` に、生成コードとして再配布される protobuf 由来物と protoc-gen-jsonif の Apache-2.0 帰属表示を追加し、libwebrtc 経由の依存ライブラリの列挙も整理する。

## 現状

`NOTICE.md` は Sora C++ SDK / Boost / libwebrtc のライセンス文言のみを収録している。

一方、実際のビルドでは `protoc` と `protoc-gen-jsonif` を利用して以下の生成コードがリポジトリに含まれ、SDK として再配布される。

- `SoraUnitySdkExamples/Assets/SoraUnitySdk/Generated/Jsonif.cs`
- `SoraUnitySdkExamples/Assets/SoraUnitySdk/Generated/SoraConf.cs`
- `SoraUnitySdkExamples/Assets/SoraUnitySdk/Generated/SoraConfInternal.cs`

これらの生成コードには protobuf の runtime 依存はないが、生成物の由来として protoc-gen-jsonif（Apache-2.0）と protobuf（BSD-3-Clause）の帰属表示があると誤解が減る。

さらに libwebrtc の内部依存として openh264 / libyuv / boringssl / libjpeg-turbo / opus 等が同梱されているが、NOTICE には触れられていない。

## 設計方針

- `NOTICE.md` に protobuf（DEPS の `PROTOBUF_VERSION`）の帰属表示を追加する
- protoc-gen-jsonif の帰属表示を追加する
- libwebrtc 経由で再配布される主要依存の帰属表示を追加する（少なくとも openh264, libyuv, boringssl）
- 各依存のライセンス条項全文を書くと肥大化するため、依存の一覧とライセンス種別（Apache-2.0 / BSD-3-Clause 等）を明記し、詳細は上流の LICENSE を参照する記述にとどめる

## 完了条件

- `NOTICE.md` に protobuf / protoc-gen-jsonif の帰属表示が追加されている
- libwebrtc 経由の再配布依存の一覧が追記されている
- Sora C++ SDK / Boost / libwebrtc の既存記述は温存されている
- `CHANGES.md` の `## develop` に `[UPDATE] NOTICE.md に protobuf / protoc-gen-jsonif の帰属表示を追加する` を追記する
