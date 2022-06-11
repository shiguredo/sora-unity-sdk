# Windows 10 x86_64 向け Sora Unity SDK を自前でビルドする

**ビルドに関する質問は受け付けていません**

## 事前準備

以下のツールをインストールしてください。

- [CMake](https://cmake.org/)
- [Visual Studio 2019 \| Visual Studio](https://visualstudio.microsoft.com/ja/vs/?rr=https%3A%2F%2Fwww.google.com%2F)
    - 動作確認は Visual Studio 2019 Community で行っています
- Python3

### Unity プラグインのビルド

PowerShell を起動し、プロジェクトのルートディレクトリで `python3 run.py windows_x86_64` を実行してください。

うまくいくと `_build\windows_x86_64\release\sora_unity_sdk\Release\SoraUnitySdk.dll` が生成されます。

## インストール

`_build\windows_x86_64\release\sora_unity_sdk\Release\SoraUnitySdk.dll` と `Sora\` を自身のプロジェクトにコピーしてください。
