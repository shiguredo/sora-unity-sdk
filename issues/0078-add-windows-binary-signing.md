# [Windows] SoraUnitySdk.dll に Authenticode 署名を行う

- Created: 2026-09-10
- Completed: {YYYY-MM-DD}
- Branch: feature/add-windows-binary-signing
- Polished: 2026-09-11

## 目的

配布する Windows 向けネイティブプラグイン `SoraUnitySdk.dll` に Authenticode 署名を行い、発行元の証明と改ざん検知を可能にする。署名のない DLL は SmartScreen の警告やウイルス対策ソフトによる誤検知・ブロックの対象になり、利用者が SDK を導入しづらくなる。

## 現状

- `.github/workflows/build.yml` の `build` ジョブ（`strategy.matrix` の `windows_x86_64`）が `windows-2022` ランナーで `python3 run.py build windows_x86_64` を実行する
- `CMakeLists.txt` は `SORA_UNITY_SDK_PACKAGE` が `windows_x86_64` の場合に `add_library(SoraUnitySdk SHARED)` で `SoraUnitySdk.dll` を生成する
- `run.py` の `_build(args)` が `_build/windows_x86_64/release/sora_unity_sdk/Release/SoraUnitySdk.dll` を `SoraUnitySdkExamples/Assets/Plugins/SoraUnitySdk/windows/x86_64/SoraUnitySdk.dll` にコピーする
- `run.py` の `_package()` が `SoraUnitySdkExamples/Assets/Plugins/SoraUnitySdk/windows/x86_64/SoraUnitySdk.dll` を `_package/SoraUnitySdk/Plugins/SoraUnitySdk/windows/x86_64/SoraUnitySdk.dll` に配置する
- `.github/workflows/build.yml` の `package` ジョブが各プラットフォームの成果物を `SoraUnitySdk.zip` にまとめ、`release` ジョブがタグ付きビルド（`contains(github.ref, 'tags/20')`）の場合に GitHub Releases へアップロードする
- `signtool` などによる署名処理は存在しない

## 設計方針

- 変更対象は `.github/workflows/build.yml`（`build` ジョブに署名・検証ステップを追加）と `CHANGES.md` とし、`run.py` は変更しない
- `windows-2022` ランナー上で `signtool` を使い、`run.py build` がコピーした `SoraUnitySdkExamples/Assets/Plugins/SoraUnitySdk/windows/x86_64/SoraUnitySdk.dll` に Authenticode 署名を行う（`run.py package` はこのファイルを配布物へコピーするため、ここに署名すればパッケージにも反映される）
- 署名は `run.py build` の後、`run.py package` の前に行い、署名済みの DLL をパッケージに含める
- 証明書（PFX）とパスワードは GitHub Secrets で管理し、実値をリポジトリやワークフローに書かない
  - `WINDOWS_CERTIFICATE`（Base64 エンコードした PFX）と `WINDOWS_CERTIFICATE_PASSWORD`
- ハッシュアルゴリズムは SHA256 を使い、RFC3161 タイムスタンプを付与する
  - 例: `signtool sign /fd SHA256 /tr <タイムスタンプ URL> /td SHA256 <DLL>`
- 署名後に `signtool verify /pa` で検証し、失敗時はジョブを失敗させる
- 証明書のインポートに一時的な証明書ストアを使う場合は、ジョブ終了時に削除する
- Secrets 未設定時の扱い
  - Secrets が設定されている場合は、タグの有無にかかわらず署名する
  - Secrets 未設定の場合、タグ付きビルド（`contains(github.ref, 'tags/20')` で GitHub Releases にアップロードされるビルド）では署名を必須とし、ジョブを失敗させる
  - Secrets 未設定の場合、タグなしの通常ビルドでは署名ステップをスキップする
- `windows_x86_64` 以外のターゲットでは署名ステップを実行しない

## 完了条件

- `windows_x86_64` のビルド成果物 `SoraUnitySdk.dll` に Authenticode 署名が付与されていること
- `signtool verify /pa` が成功すること
- 署名済み DLL を含む `SoraUnitySdk.zip` が GitHub Releases にアップロードされること
- タグ付きビルドで Secrets 未設定の場合にジョブが失敗すること
- 証明書などの秘密情報がリポジトリや CI ログに残らないこと
- `CHANGES.md` の `## develop` の `### misc` に `[ADD]` エントリを追記すること
