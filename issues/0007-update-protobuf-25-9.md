# proto を 25.9 に上げる

- Created: 2026-08-25
- Completed: {YYYY-MM-DD}
- Branch: feature/update-protobuf-25-9
- Polished: {YYYY-MM-DD}

## 目的

Sora Unity SDK のビルドで使用する protobuf の compiler(protoc)を `25.6` から `25.9` に上げる。

## 現状

`DEPS` の `PROTOBUF_VERSION` が `25.6`。`protoc` は `proto/sora_conf.proto` と `proto/sora_conf_internal.proto` から C++ / Unity 用の生成コードを作るビルド専用ツールとして使われる。

## 設計方針

- `DEPS` の `PROTOBUF_VERSION` を `25.9` に変更する
- 生成ヘッダはビルド時に `CMAKE_CURRENT_BINARY_DIR` 配下へ生成されるためコミット対象にはならない(バージョン変更のみでよい)
- `PROTOC_GEN_JSONIF_VERSION` は変更しない

## 完了条件

- `DEPS` の `PROTOBUF_VERSION` が `25.9` になっている
- 検証ケースの全項目を満たす
- `CHANGES.md` の `## develop` に `[UPDATE]` を追記する

## 検証ケース

- `python3 run.py build macos_arm64` で protoc `25.9` が正しく入手され、`proto/sora_conf.proto` / `proto/sora_conf_internal.proto` から生成される `sora_conf.json.h` / `sora_conf_internal.json.h` がコンパイルエラーなく生成されること
- `python3 run.py package` でパッケージ作成が正常に終了すること
- Unity エディタ(macOS)で `SoraUnitySdkExamples/Assets/SoraSample.cs` を起動し、通常どおり Sora のシグナリングに接続できること
  - 期待: protoc `25.9` で生成された C# の jsonif コード経由で接続設定 JSON が正しく組み立てられ、Sora へ送信されて接続できること
  - 他ターゲット(ios / android / linux / windows)は push 時の `.github/workflows/build.yml` の CI が DEPS 変更を検知して自動でビルドするため、手動検証は macOS で代表させる

## 解決方法

1. `DEPS` の `PROTOBUF_VERSION` を `25.9` に変更する
2. `python3 run.py build macos_arm64` でビルドと生成コードを確認する
3. Unity エディタ の `SoraSample.cs` で Sora への実接続を確認する
4. `CHANGES.md` に `[UPDATE] proto を 25.9 に上げる` を追記する
