# macOS x86_64 向け Sora Unity SDK を自前でビルドする

## 事前準備

以下のツールをインストールしてください。

- Xcode
- [CMake](https://cmake.org/)

### 依存ライブラリのビルド

コマンドラインで `install_tools.sh` を実行してください。
libwebrtc と関連ツールのダウンロードも含むので時間がかかります。

```
$ sh install_tools.sh
```

### Unity プラグインのビルド

コマンドラインで `cmake.sh` を実行してください。

```
$ sh cmake.sh
```

ビルドに成功すると `build/SoraUnitySdk.bundle` が生成されます。

## インストール

`build/SoraUnitySdk.bundle` と `Sora/Sora.cs` を任意のプロジェクトの Assets にコピーしてください。

