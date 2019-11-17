# 開発者向けドキュメント (Windows)

## 環境構築

### 事前準備

以下のツールをインストールしてください。

- [CMake](https://cmake.org/)
- [Visual Studio 2019 \| Visual Studio](https://visualstudio.microsoft.com/ja/vs/?rr=https%3A%2F%2Fwww.google.com%2F)
    - 動作確認は Visual Studio 2019 Community で行っています

### 依存ライブラリのビルド

`.\install_tools.bat` をダブルクリックで実行するか、コマンドプロンプトから実行してください。

### Unity プラグインのビルド

`.\cmake.bat` をダブルクリックで実行するか、コマンドプロンプトから実行してください。

うまくいくと `build\Release\SoraUnitySdk.dll` が生成されます。

## インストール

`build\Release\SoraUnitySdk.dll` と `Sora\Sora.cs` を自身のプロジェクトにコピーしてください。
