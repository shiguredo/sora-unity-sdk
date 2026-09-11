# [macOS] SoraUnitySdk.bundle にコード署名を行う

- Created: 2026-09-10
- Completed: {YYYY-MM-DD}
- Branch: feature/add-macos-binary-signing
- Polished: 2026-09-11

## 目的

配布する macOS 向けネイティブプラグイン `SoraUnitySdk.bundle` に Developer ID でコード署名を行い、Gatekeeper の検証を通過できるようにする。署名のないプラグインは、Hardened Runtime を有効にしたアプリへ読み込めなかったり、起動時に警告が表示されたりする。配布物として信頼できる状態にする。

## 現状

- `.github/workflows/build.yml` の `macos_arm64` ジョブが `macos-15` ランナーで `python3 run.py build macos_arm64` を実行する
- `CMakeLists.txt` は `SORA_UNITY_SDK_PACKAGE` が `macos_x86_64` / `macos_arm64` の場合に `add_library(SoraUnitySdk MODULE)` と `set_target_properties(SoraUnitySdk PROPERTIES BUNDLE TRUE)` で `SoraUnitySdk.bundle` を生成する
- `run.py` の `_build(args)` が `_build/macos_arm64/release/sora_unity_sdk/SoraUnitySdk.bundle` を `SoraUnitySdkExamples/Assets/Plugins/SoraUnitySdk/macos/arm64/SoraUnitySdk.bundle` にコピーする
- `run.py` の `_package()` が `_package/SoraUnitySdk/Plugins/SoraUnitySdk/macos/arm64/SoraUnitySdk.bundle` に配置する
- `macos_x86_64` は `run.py` の `AVAILABLE_TARGETS` に含まれておらず、現在 CI がビルドする macOS 向けバイナリは `macos_arm64` のみ
- `codesign` などによる署名処理は存在しない

## 設計方針

- 変更対象は `.github/workflows/build.yml`（`build` ジョブに署名・検証ステップを追加）と `CHANGES.md` とし、`run.py` は変更しない
- `macos-15` ランナー上で `codesign` を使い、`run.py build` がコピーした `SoraUnitySdkExamples/Assets/Plugins/SoraUnitySdk/macos/arm64/SoraUnitySdk.bundle` に Developer ID Application 証明書で署名する（`run.py package` はこのファイルを配布物へコピーするため、ここに署名すればパッケージにも反映される）
- 署名は `run.py build` の後、`run.py package` の前に行い、署名済みの bundle をパッケージに含める
- Hardened Runtime を有効にし、タイムスタンプを付与する
  - 例: `codesign --force --timestamp --options runtime --sign "<証明書名>" SoraUnitySdk.bundle`
- 証明書（p12）とパスワード、一時キーチェーンのパスワードは GitHub Secrets で管理し、実値をリポジトリやワークフローに書かない
  - 例: `MACOS_CERTIFICATE`（Base64 エンコードした p12）と `MACOS_CERTIFICATE_PASSWORD`、`MACOS_KEYCHAIN_PASSWORD`
- 証明書は一時キーチェーンにインポートし、ジョブ終了時に削除する
- 署名後に `codesign --verify --deep --strict` で検証し、失敗時はジョブを失敗させる
- 公証（notarization）は本 issue のスコープ外とし、必要なら別 issue とする
- Secrets 未設定時の扱い
  - Secrets が設定されている場合は、タグの有無にかかわらず署名する
  - Secrets 未設定の場合、タグ付きビルド（`contains(github.ref, 'tags/20')` で GitHub Releases にアップロードされるビルド）では署名を必須とし、ジョブを失敗させる
  - Secrets 未設定の場合、タグなしの通常ビルドでは署名ステップをスキップする
- `macos_arm64` 以外のターゲットでは署名ステップを実行しない（`ios` も `macos-15` ランナーでビルドされるため、`matrix.target` で条件分岐する）

## 完了条件

- `macos_arm64` のビルド成果物 `SoraUnitySdk.bundle` に Developer ID 署名が付与されていること
- `codesign --verify --deep --strict` が成功すること
- 署名済み bundle を含む `SoraUnitySdk.zip` が GitHub Releases にアップロードされること
- 証明書などの秘密情報がリポジトリや CI ログに残らないこと
- `CHANGES.md` の `## develop` の `### misc` に `[ADD]` エントリを追記すること
