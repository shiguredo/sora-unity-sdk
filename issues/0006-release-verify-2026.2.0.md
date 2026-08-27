# 2026.2.0 リリース検証をする

- Created: 2026-08-25
- Completed: {YYYY-MM-DD}
- Milestone: 2026.2.0
- Polished: 2026-08-25
- Updated: 2026-08-25

## 目的

sora-unity-sdk の 2026.2.0 リリース前検証を完了させる。master (2026.1.0) からの差分として開発中の変更がすべて develop に揃っていることを確認し、依存する sora-cpp-sdk の 2026.2.0 / 2026.2.1 更新に伴う挙動変化をリリース前に検証する。

本 issue はリリース前検証の追跡を担う。個別の依存追従やドキュメント修正は各 issue に分かれており、ここでは検証と完了条件を取りまとめる。リリースの実行（VERSION の確定、release ブランチからの master マージ・タグ付与、GitHub Release へのアップロード）は本 issue の対象外とする。

## 現状

- リポジトリの `develop` ブランチの VERSION は `2026.2.0-canary.7`
- `DEPS` の `SORA_CPP_SDK_VERSION` は `2026.2.1`。sora-cpp-sdk 本体は 2026.2.0 (2026-08-14) と 2026.2.1 (2026-08-18) がリリース済み
- master は 2026.1.0 で、develop の差分が本リリースの対象
- 2026.2.0 リリースに伴う依存追従は `issues/closed/0004-update-sora-cpp-sdk-2026.2.1.md`、対応 OS の見直しは `issues/0005-update-readme-supported-os-versions.md`、Open Proto の追従は `issues/closed/0007-update-protobuf-25-9.md` で個別に管理されている
- 依存追従 (0004) は PR #192 (feature/update-sora-cpp-sdk-2026.2.1) で実装・検証済みで develop へマージ済み。Open Proto の追従 (0007) は PR #193 でマージ済み。本 issue では、マージ済みの develop が 2026.2.0 としてリリース可能な状態かを確認する

### 依存バージョンの現状

sora-cpp-sdk 2026.2.1 は 2026.2.0 のパッチリリースで、追加差分は WebSocket 切断時クラッシュ修正の 1 件のみ。依存追従 (0004 / PR #192) と Open Proto の追従 (0007 / PR #193) は develop へ反映済みで、DEPS はリリース対象の値に一致する。リリース対象となる依存バージョンは次のとおり。

| DEPS のキー | 値 |
| --- | --- |
| `SORA_CPP_SDK_VERSION` | `2026.2.1` |
| `WEBRTC_BUILD_VERSION` | `m150.7871.3.1` |
| `BOOST_VERSION` | `1.92.0` |
| `CMAKE_VERSION` | `4.4.2` |
| `PROTOBUF_VERSION` | `25.9` |

### 検証状況

検証はすべて HD (1280x720) / `videoBitRate` 8000 で実施した。

- ビルドバイナリ（`python3 run.py build` の成果物）での送受信を全プラットフォームで実施済み
- macOS (`macos_arm64`): sendrecv / sendrecv + simulcast / DataChannel シグナリング / 独自 CA 指定の接続をすべて確認済み
- iOS: sendrecv / sendrecv + simulcast を確認済み。なお設定不要（`SoraUnitySdkPostProcessor.cs` への `Security.framework` 追加なし）で動作することを確認済みのため、`Security.framework` は問題なし
- Android: sendrecv / sendrecv + simulcast を確認済み
- Windows (`windows_x86_64`): ビルドバイナリで sendrecv を確認済み
- Ubuntu 24.04 (`ubuntu-24.04_x86_64`): ビルドバイナリで sendrecv を確認済み
- Editor: 接続・送受信・切断の動作を確認済み
- カメラ切り替え（DeviceCamera ⇔ UnityCamera）による Unity Camera の動作を確認済み

## リリース対象の変更点

master (2026.1.0) からの差分で、2026.2.0 に含まれる変更。

- sora-cpp-sdk を `2026.2.1` に追従する
  - libwebrtc を `m150.7871.3.1` に上げる
  - BOOST_VERSION を `1.92.0`、CMAKE_VERSION を `4.4.2` に上げる
  - `src/sora.cpp` について、libwebrtc m150 で `stream_ids()` が削除されたため `streams()` を使うよう修正する
- Open Proto を `25.9` に上げる（`PROTOBUF_VERSION`）
- sora-cpp-sdk 2026.2.0 の TLS 検証のシステム CA 化に伴う、iOS での OS システム CA 経由の TLS 検証への切り替え
  - iOS は sandbox 制約により `SecTrustEvaluateWithError` に検証を委譲する方式で `Security.framework` が必要になる。ただし Unity の iOS ビルドはプロジェクト生成時に標準のシステムフレームワーク群（`Security.framework` を含む）を自動リンクするため、`SoraUnitySdkExamples/Assets/SoraUnitySdk/Editor/SoraUnitySdkPostProcessor.cs` への `Security.framework` 追加は不要とする（0004 で生成される Xcode プロジェクトの Frameworks phase に含まれることを確認済み）
- sora-cpp-sdk 2026.2.0 の `SoraClientContext` ABI 変更（`ConnectionContext::MediaEngineReference` 保持）への追従。現依存の 2026.2.1 に反映済みで、追加のソース変更は不要
- sora-cpp-sdk 2026.2.0 の NVIDIA Pascal 世代以前の GPU サポート廃止に伴う利用者への影響周知（`README.md` の対応機能への記載。現状 develop の `README.md` には未反映のため要対応）
- README の対応 OS バージョンを実態（iOS 14 / macOS 14 / Android 10）に合わせる
- CI の整理: GitHub Actions のバージョン更新、dependabot.yml 追加、slack-notify 追加、`claude.yml` と `.github/copilot-instructions.md` の削除

## 範囲外

- H.265 / Opus params / Android 縦向きなどの機能追加 (`issues/0001-3`) は 2026.2.0 には含めない。実装が develop に載っていないため、本リリース検証の対象外とする
- リリースの実行（`VERSION` を `2026.2.0-canary.7` から `2026.2.0` へ変更、`CHANGES.md` の `## develop` を `## 2026.2.0` にしてリリース日を記載、release ブランチから master へマージしてタグを付与、CI の release ジョブによる GitHub Release へのアップロードと slack-notify）は本 issue（リリース前検証）の対象外とし、リリース実行側で行う
- 2026.2.0 以降で予定される機能は対象外

## 検証バリエーション

本検証は sora-cpp-sdk 2026.2.1 への追従で既存の挙動が壊れていないことを確認するリグレッション検証である。実機接続検証は SoraUnitySdkExamples のサンプルアプリを各プラットフォームで実行し、Sora サーバーへの接続・送受信・切断を行う。動画のハードウェアアクセラレータは、各プラットフォームで利用可能な実装が自動で選ばれる（Windows / Ubuntu は NVIDIA Video Codec または Intel VPL、macOS / iOS / Android は内部実装のハードウェア）。

なお、sora-cpp-sdk 2026.2.1 への依存追従（全ターゲットのビルド、iOS の `Security.framework` 自動リンク、各プラットフォームの動作検証、CHANGES.md の更新）は PR #192 (closed 済みの `issues/closed/0004-update-sora-cpp-sdk-2026.2.1.md` に記録) で確認済み。本 issue では、マージ済みの develop が 2026.2.0 としてリリース可能な状態かを確認する。

### 1. ビルドとパッケージ（CI）

- 全ターゲットのビルド (CI)
  - 確認内容: `python3 run.py build <target>` が全ターゲット (`windows_x86_64` / `macos_arm64` / `ios` / `ubuntu-22.04_x86_64` / `ubuntu-24.04_x86_64` / `android`) で成功し、CI (`build.yml`) が green になること
  - 結果: PR #192 で確認済み。develop へマージされた状態で改めて CI が green になることを確認する
- パッケージ生成
  - 確認内容: `python3 run.py package` で各ターゲットのライブラリが `_package/` に生成されること
  - 結果: PR #192 で確認済み。マージ後の develop で再確認する
- iOS の `Security.framework` 自動リンク
  - 確認内容: Unity の iOS ビルドで生成される Xcode プロジェクトに `Security.framework` が含まれること（`SoraUnitySdkPostProcessor.cs` への追加なし）
  - 結果: PR #192 で確認済み。実機の iOS でも設定なし（`SoraUnitySdkPostProcessor.cs` への追加なし）で動作することを確認済み

### 2. 接続・送受信のリグレッション

`src/sora.cpp` の `OnTrack` は、libwebrtc m150 で削除された `stream_ids()` の代わりに `streams()` を使うよう修正されており、受信映像の接続 ID は `streams()` の先頭要素から導出される（`streams.empty() ? "" : streams[0]->id()`）。次のシナリオを、各プラットフォーム × 利用可能なハードウェアアクセラレータで確認する。

- sendrecv で送受信する
  - 対象プラットフォーム: `macos_arm64` / `windows_x86_64` / `android` / `ios` / `ubuntu-24.04_x86_64`
  - 利用するハードウェアアクセラレータ: NVIDIA Video Codec / Intel VPL（利用可能な実装）
  - 確認内容: Sora サーバーへの TLS 接続、カメラ映像と受信映像の追尾、受信映像ごとの接続 ID の識別が正常に動作すること
  - 結果: macOS / iOS / Android / Windows / Ubuntu 24.04 で確認済み（TLS 接続・カメラ映像と受信映像の追尾・接続 ID の識別が正常に動作することを確認。HD (1280x720) / `videoBitRate` 8000 で実施）
- sendrecv + simulcast で送受信する
  - 対象プラットフォーム: `macos_arm64` / `windows_x86_64` / `android` / `ios` / `ubuntu-24.04_x86_64`
  - 利用するハードウェアアクセラレータ: NVIDIA Video Codec / Intel VPL（利用可能な実装）
  - 確認内容: `Simulcast` / `SimulcastRequestRid` で複数層を受信しても接続 ID が一貫し、受信映像の追尾が正常に動作すること
  - 結果: macOS / iOS / Android で確認済み（複数層を受信しても接続 ID が一貫し、受信映像の追尾が正常に動作することを確認）
- sendrecv（DataChannel シグナリング）で送受信して切断する
  - 確認内容: `dataChannelSignaling` を有効にして Sora へ接続・送受信・切断し、WebSocket close 完了と DataChannel close 通知の順序が入れ替わっても SIGSEGV しないこと（sora-cpp-sdk 2026.2.1 のクラッシュ修正の回帰）
  - 結果: PR #192 で確認済み。macOS (develop) でも確認済み
- カメラ切り替え（Unity Camera）で送受信する
  - 確認内容: `SoraSample` のカメラ切り替え（DeviceCamera ⇔ UnityCamera）で映像を送信し、Unity Camera の映像が正しく Sora へ送信されること
  - 結果: Editor で確認済み（DeviceCamera と UnityCamera の切り替えで送受信が正常に動作することを確認。HD (1280x720) / `videoBitRate` 8000 で実施）
- 独自 CA を指定した接続（プラスアルファ）
  - 前提: Sora サーバー側（nginx の TLS 証明書）を独自 CA で発行した証明書にし、クライアント側で `Config.CACert` (ca_cert) にその独自 CA の PEM を指定する。サーバー側とクライアント側の両方の設定が揃って初めて検証できる
  - 確認内容: 独自 CA を使う環境で `Config.CACert` (ca_cert) に PEM を明示指定して Sora へ接続できること
  - 結果: macOS で確認済み（独自 CA 発行の証明書にした Sora サーバーへ `Config.CACert` に PEM を明示指定して接続できることを確認）
- ハードコード CA への非依存
  - 確認内容: システム CA 化後も従来のハードコード CA (`isrg_root` / `lets_encrypt_r3`) への依存が残っていないこと

### 3. NVIDIA GTX 10 シリーズ（Pascal 世代以前）のハードウェアエンコーダー（本 SDK では未検証）

- sora-cpp-sdk 2026.2.0 で NVIDIA Pascal 世代以前（GTX 10 シリーズ）の GPU サポートが廃止されたことにより、ハードウェアエンコーダー / デコーダーが使えずソフトウェアエンコードへフォールバックする挙動
  - 確認内容: GTX 10 シリーズでハードウェアエンコーダー / デコーダーが利用できず、ソフトウェアエンコードへフォールバックすること
  - 結果: 本 SDK では検証していない。本 SDK の検証環境に該当世代の NVIDIA GPU がないため。挙動は sora-cpp-sdk が制御する既知挙動とし、検証は sora-cpp-sdk 側に委ねる
  - 備考: `VideoCodecImplementation.NvidiaVideoCodec` を利用する GTX 10 シリーズでハードウェアエンコーダーが使えなくなる旨は `README.md`（対応機能）で周知する想定だが、現状 develop の `README.md` に未反映のため要対応（0004 の設計方針は周知するとしていたが、0004 完了時の記録にも反映がなく未達）

### 4. ドキュメントと変更履歴

- `CHANGES.md`: develop エントリを 2026.2.1 向けに更新し、リリース時に `## 2026.2.0` と `**リリース日**: YYYY-MM-DD` へ確定する（0004 の完了条件）
- README / `SoraUnitySdkExamples/README.md`: 対応プラットフォームが実態（iOS 14 / macOS 14 / Android 10）に一致する（0005 の完了条件）
- リリースノートと実際の変更内容の整合性を確認する

## 完了条件

- 全 6 ターゲットで `python3 run.py build` と `python3 run.py package` が成功し、CI が green になる
- iOS で `Security.framework` が自動でリンクされること（`SoraUnitySdkPostProcessor.cs` への追加なし）を確認できる
- sendrecv / sendrecv + simulcast（各プラットフォーム × 利用可能なハードウェアアクセラレータ）での送受信、独自 CA 指定での接続、DataChannel シグナリングでの切断において、sora-cpp-sdk 2026.2.1 の挙動が従来どおり（回帰していない）であることを確認できる
- `CHANGES.md` の develop エントリと README の対応 OS がリリース内容・実態（iOS 14 / macOS 14 / Android 10）と整合する（`VERSION` の確定と `## develop` から `## 2026.2.0` への変換はリリース実行側の作業で本 issue の対象外）
- NVIDIA 周知（GTX 10 シリーズでハードウェアエンコーダーが使えなくなる旨）が `README.md`（対応機能）に記載されていることを確認する（現状未反映のため要対応）
- GTX 10 シリーズ（Pascal 世代以前）でハードウェアエンコーダー / デコーダーが利用できず、ソフトウェアエンコードへフォールバックする挙動は、sora-cpp-sdk 2026.2.0 のサポート廃止によるもので、本 SDK の検証環境に該当世代の NVIDIA GPU がないため本 SDK では検証しない