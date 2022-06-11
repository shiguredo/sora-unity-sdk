# macOS x86_64 向け Sora Unity SDK を自前でビルドする

**ビルドに関する質問は受け付けていません**

## 事前準備

以下のツールをインストールしてください。

- Xcode
- [CMake](https://cmake.org/)

### Unity プラグインのビルド

コマンドラインで `python3 run.py macos_arm64` を実行してください。

```
$ python3 run.py macos_arm64
```

ビルドに成功すると `_build/macos_arm64/release/sora_unity_sdk/SoraUnitySdk.bundle` が生成されます。

## インストール

`_build/macos_arm64/release/sora_unity_sdk/SoraUnitySdk.bundle` と `Sora/` を任意のプロジェクトの Assets にコピーしてください。

