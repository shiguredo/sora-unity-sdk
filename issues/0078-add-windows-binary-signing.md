# [Windows] SoraUnitySdk.dll に Authenticode 署名を行う

- Created: 2026-09-10
- Completed: {YYYY-MM-DD}
- Branch: feature/add-windows-binary-signing
- Polished: {YYYY-MM-DD}

## 目的

配布する Windows 向けネイティブプラグイン `SoraUnitySdk.dll` に Authenticode 署名を行い、発行元の証明と改ざん検知を可能にする。署名のない DLL は SmartScreen の警告やウイルス対策ソフトによる誤検知・ブロックの対象になり、利用者が SDK を導入しづらくなる。

## 現状

- `.github/workflows/build.yml` の `windows_x86_64` ジョブが `windows-2022` ランナーで `python3 run.py build windows_x86_64` を実行する
- `CMakeLists.txt` は `SORA_UNITY_SDK_PACKAGE` が `windows_x86_64` の場合に `add_library(SoraUnitySdk SHARED)` で `SoraUnitySdk.dll` を生成する
- `run.py` の `_build(args)` が `_build/sora_unity_sdk/<構成>/SoraUnitySdk.dll` を `SoraUnitySdkExamples/Assets/Plugins/SoraUnitySdk/windows/x86_64/SoraUnitySdk.dll` にコピーする
- `run.py` の `_package()` が `_package/SoraUnitySdk/Plugins/SoraUnitySdk/windows/x86_64/SoraUnitySdk.dll` に配置する
- `.github/workflows/build.yml` の `package` ジョブが各プラットフォームの成果物を `SoraUnitySdk.zip` にまとめ、タグ付きビルドでは GitHub Releases にアップロードする
- `signtool` などによる署名処理は存在しない

## 設計方針

- `windows-2022` ランナー上で `signtool` を使い、`SoraUnitySdk.dll` に Authenticode 署名を行う
- 署名は `run.py build` の後、`run.py package` の前に行い、署名済みの DLL をパッケージに含める
- 証明書（PFX）とパスワードは GitHub Secrets で管理し、実値をリポジトリやワークフローに書かない
  - 例: `WINDOWS_CERTIFICATE`（Base64 エンコードした PFX）と `WINDOWS_CERTIFICATE_PASSWORD`
- ハッシュアルゴリズムは SHA256 を使い、タイムスタンプを付与する
  - 例: `signtool sign /fd SHA256 /tr <タイムスタンプ URL> /td SHA256 <DLL>`
- 署名後に `signtool verify /pa` で検証し、失敗時はジョブを失敗させる
- 証明書のインポートに一時的な証明書ストアを使う場合は、ジョブ終了時に削除する
- 署名に必要な Secrets が未設定の場合の扱い（スキップするか失敗させるか）を決める

## 完了条件

- `windows_x86_64` のビルド成果物 `SoraUnitySdk.dll` に Authenticode 署名が付与されていること
- `signtool verify /pa` が成功すること
- 署名済み DLL を含む `SoraUnitySdk.zip` が GitHub Releases にアップロードされること
- 証明書などの秘密情報がリポジトリや CI ログに残らないこと
- `CHANGES.md` の `## develop` に `[ADD]` エントリを追記すること
