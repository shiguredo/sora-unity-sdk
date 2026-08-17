# Sora C++ SDK を 2026.2.0 に上げる

- Created: 2026-08-17
- Completed: {YYYY-MM-DD}
- Branch: feature/update-sora-cpp-sdk-2026.2.0
- Polished: {YYYY-MM-DD}

## 目的

sora-cpp-sdk 2026.2.0 がリリースされたため、sora-unity-sdk の依存を最新リリースに追従する。

## 現状

DEPS は `SORA_CPP_SDK_VERSION=2026.2.0-canary.18` を参照しており、canary.18 から 2026.2.0 までの差分が未反映である。

## 設計方針

DEPS を 2026.2.0 に合わせて更新する。

- `SORA_CPP_SDK_VERSION`: `2026.2.0-canary.18` → `2026.2.0`
- `WEBRTC_BUILD_VERSION`: `m150.7871.3.0` → `m150.7871.3.1`
- `BOOST_VERSION`: `1.91.0` → `1.92.0`
- `CMAKE_VERSION`: `4.3.2` → `4.4.2`

今回の追従で sora-unity-sdk 側に新規対応が必要なのは TLS 検証のシステム CA 化のみである。

なお、`SoraClientContext` の ABI 変更 (MediaEngineReference 保持) と `BOOST_ASIO_ENABLE_VERSION_NAMESPACE` の有効化は、いずれも現依存の 2026.2.0-canary.18 に既に含まれており、今回の追従で追加対応は不要である。

- TLS 検証の信頼ストアを OS のシステム CA に切り替える
  - iOS は `SecTrustEvaluateWithError` による検証委譲のため、iOS アプリのビルドに `Security.framework` の追加が必要になる
  - `SoraUnitySdkPostProcessor.cs` の `AddFrameworkToProject` に `Security.framework` を追加する
  - 独自 CA を使う場合は `Config.CACert` (ca_cert) に PEM を明示指定する（既存機能で対応済み）
  - sora-cpp-sdk 2026.2.0 の TLS システム CA 化は iOS 14 以降が対象のため、iOS 13 端末での TLS 検証の挙動を確認する

## 完了条件

- 全プラットフォーム (windows_x86_64 / macos_arm64 / ubuntu-22.04_x86_64 / ubuntu-24.04_x86_64 / ios / android) でビルドが成功する
- SoraUnitySdkExamples の iOS ビルドで生成される Xcode プロジェクトに `Security.framework` が追加される
  - CI は `run.py build` によるライブラリビルドのみで `SoraUnitySdkPostProcessor.cs` が実行されないため、Unity での iOS ビルドによる確認が必要
- `CHANGES.md` の develop に [UPDATE] エントリが追記されている

## 解決方法

1. DEPS を更新する
2. `SoraUnitySdkPostProcessor.cs` に `Security.framework` を追加する
3. 全プラットフォームでビルドを検証する
4. SoraUnitySdkExamples を iOS ビルドし、生成された Xcode プロジェクトに `Security.framework` が含まれることを確認する
5. `CHANGES.md` の develop に追記する
