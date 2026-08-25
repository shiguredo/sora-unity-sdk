# 2026.2.0 リリース検証をする

- Created: 2026-08-25
- Completed: {YYYY-MM-DD}
- Milestone: 2026.2.0
- Polished: {YYYY-MM-DD}

## 目的

sora-unity-sdk の 2026.2.0 リリースを完了させる。master (2026.1.0) からの差分として開発中の変更がすべて develop に揃っていることを確認し、依存する sora-cpp-sdk の 2026.2.0 / 2026.2.1 更新に伴う挙動変化をリリース前に検証する。

本 issue はリリース検証の追跡を担う。個別の依存追従やドキュメント修正は各 issue に分かれており、ここでは全体の検証と完了条件を取りまとめる。

## 現状

- リポジトリの `develop` ブランチの VERSION は `2026.2.0-canary.5`
- `DEPS` の `SORA_CPP_SDK_VERSION` は `2026.2.0-canary.18`。一方で sora-cpp-sdk 本体は 2026.2.0 (2026-08-14) と 2026.2.1 (2026-08-18) がリリース済み
- master は 2026.1.0 で、develop の差分が本リリースの対象
- 2026.2.0 リリースに伴う依存追従は `issues/0004-update-sora-cpp-sdk-2026.2.1.md`、対応 OS の見直しは `issues/0005-update-readme-supported-os-versions.md` で個別に管理されている
- 依存追従 (0004) は PR #192 (feature/update-sora-cpp-sdk-2026.2.1) で実装・検証済み。本 issue は PR #192 のマージ後に着手する前提のため、DEPS は `SORA_CPP_SDK_VERSION=2026.2.1` 等に更新された状態から作業する

### 依存バージョンの現状

sora-cpp-sdk 2026.2.1 は 2026.2.0 のパッチリリースで、追加差分は WebSocket 切断時クラッシュ修正の 1 件のみ。リリース対象となる依存バージョンは次のとおり。

| DEPS のキー | develop の現在値 | リリースで参照する値 |
| --- | --- | --- |
| `SORA_CPP_SDK_VERSION` | `2026.2.0-canary.18` | `2026.2.1` |
| `WEBRTC_BUILD_VERSION` | `m150.7871.3.0` | `m150.7871.3.1` |
| `BOOST_VERSION` | `1.91.0` | `1.92.0` |
| `CMAKE_VERSION` | `4.3.2` | `4.4.2` |

## リリース対象の変更点

master (2026.1.0) からの差分で、2026.2.0 に含まれる変更。

- sora-cpp-sdk を 2026.2.0-canary.18 から 2026.2.1 に追従する
  - libwebrtc を `m150.7871.3.1` に上げる
  - BOOST_VERSION を `1.92.0`、CMAKE_VERSION を `4.4.2` に上げる
  - `src/sora.cpp` について、libwebrtc m150 で `stream_ids()` が削除されたため `streams()` を使うよう修正する
- sora-cpp-sdk 2026.2.0 の TLS 検証のシステム CA 化に伴う、iOS での OS システム CA 経由の TLS 検証への切り替え
  - `Security.framework` は sora-cpp-sdk の `CMakeLists.txt` で macOS / iOS ターゲットに `-framework Security` が `PUBLIC` リンクオプションとして既に指定されており、アプリが sora ライブラリをリンクする際に伝播する。このため `SoraUnitySdkExamples/Assets/SoraUnitySdk/Editor/SoraUnitySdkPostProcessor.cs` の `OnPostprocessBuild` への `Security.framework` 追加は不要とする
- sora-cpp-sdk 2026.2.0 の `SoraClientContext` ABI 変更（`ConnectionContext::MediaEngineReference` 保持）への追従。現依存の 2026.2.0-canary.18 に反映済みで、追加のソース変更は不要
- sora-cpp-sdk 2026.2.0 の NVIDIA Pascal 世代以前の GPU サポート廃止に伴う利用者への影響周知（`CHANGES.md`）
- README の対応 OS バージョンを実態（iOS 14 / macOS 14 / Android 10）に合わせる
- CI の整理: GitHub Actions のバージョン更新、dependabot.yml 追加、slack-notify 追加、`claude.yml` と `.github/copilot-instructions.md` の削除

## 範囲外

- H.265 / Opus params / Android 縦向きなどの機能追加 (`issues/0001-3`) は 2026.2.0 には含めない。実装が develop に載っていないため、本リリース検証の対象外とする
- 2026.2.0 以降で予定される機能は対象外

## 検証項目

sora-cpp-sdk 2026.2.1 への依存追従（全ターゲットのビルド、iOS の `Security.framework` 自動リンク、各プラットフォームの動作検証、CHANGES.md の更新）は PR #192 (closed 済みの `issues/closed/0004-update-sora-cpp-sdk-2026.2.1.md` に記録) で確認済み。本 issue では、PR #192 のマージ後に develop が 2026.2.0 としてリリース可能な状態かを確認する。

### 1. 全プラットフォームのビルドとパッケージ

- PR #192 のマージ後に、`python3 run.py build <target>` が次の全ターゲットで成功する
  - `windows_x86_64` / `macos_arm64` / `ios` / `ubuntu-22.04_x86_64` / `ubuntu-24.04_x86_64` / `android`
- `python3 run.py package` で各ターゲットのライブラリが `_package/` に生成される
- CI (`build.yml` の build ジョブ) が全マトリックスで green になることを確認する
- 上記は PR #192 で確認済みだが、develop へマージされた状態で改めて CI が green になることを確認する

### 2. libwebrtc m150 の `streams()` 対応

`src/sora.cpp` の `OnTrack` 内で `transceiver->receiver()->streams()` を使うよう修正されている。マルチストリーム送受信時に次を確認する。

- `streams()` の先頭要素から connection_id が正しく導出され、受信映像ごとの識別が保たれる
- `streams()` が空のときは空文字が使われ、接続 ID が欠落してもクラッシュしない（`streams.empty() ? "" : streams[0]->id()`）
- sendrecv / multistream シーンで、カメラ映像と受信映像の追尾が正常に動作する

### 3. TLS 検証のシステム CA 化

sora-cpp-sdk 2026.2.0 で信頼ストアが OS のシステム CA に切り替わったことによる挙動確認。各プラットフォームの接続確認は PR #192 で実施済みだが、マージ後の develop でも同一の確認をする。

- 既定設定で Sora へ TLS 接続できる（システム CA 経由の検証で成功する）
- 独自 CA を使う環境で `Config.CACert` (ca_cert) に PEM を明示指定して接続できる
- iOS で `Security.framework` が自動でリンクされること（Unity が iOS ビルド時に標準のシステムフレームワーク群を自動リンクする。`SoraUnitySdkPostProcessor.cs` への追加は不要）
- 従来のハードコード CA (isrg_root / lets_encrypt_r3) への依存が残っていないこと

### 4. sora-cpp-sdk 2026.2.1 の切断クラッシュ修正の確認

DataChannel シグナリング利用時に、切断（WebSocket close 完了と DataChannel close 通知の順序入替え）で SIGSEGV しないことを確認する。sora-cpp-sdk 内部の修正であり Unity SDK 側の変更はないが、リリース前に該当経路の切断リグレッションを確認する。

### 5. ハードウェアエンコーダーの利用可能範囲

NVIDIA Pascal 世代以前 (GTX 10 シリーズ) ではハードウェアエンコーダー / デコーダーが利用できなくなる。利用可能な NVIDIA 世代での動作確認と、サポート対象外でハードウェアエンコーダーが使えない場合にソフトウェアエンコードへフォールバックすることを確認する。

### 6. ドキュメントと変更履歴

- `CHANGES.md` の develop エントリを 2026.2.1 向けに更新し、リリース時に `## 2026.2.0` と `**リリース日**: YYYY-MM-DD` へ確定する（0004 の完了条件）
- README / `SoraUnitySdkExamples/README.md` の対応プラットフォームが実態（iOS 14 / macOS 14 / Android 10）に一致する（0005 の完了条件）
- リリースノートと実際の変更内容の整合性を確認する

### 7. リリース作業

- `VERSION` を `2026.2.0-canary.5` から `2026.2.0` へ変更する
- `CHANGES.md` の `## develop` を `## 2026.2.0` に変更し、リリース日を記載する
- リリースブランチ (`release/2026.2.0`) から master へマージし、タグを付与する
- CI の release ジョブで `SoraUnitySdk.zip` が GitHub Release にアップロードされ、slack-notify が動作することを確認する

## 完了条件

- 全 6 ターゲットで `python3 run.py build` と `python3 run.py package` が成功し、CI が green になる
- iOS で `Security.framework` が自動でリンクされること（`SoraUnitySdkPostProcessor.cs` への追加なし）を確認できる
- TLS 接続・マルチストリーム・切断において、sora-cpp-sdk 2026.2.1 の挙動が正しいことを確認できる
- `CHANGES.md` と `VERSION` が 2026.2.0 として確定され、README の対応 OS が実態と一致する
- GitHub Release にパッケージが公開され、slack-notify による通知が完動する
