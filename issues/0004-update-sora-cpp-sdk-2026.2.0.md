# Sora C++ SDK を 2026.2.0 に上げる

- Created: 2026-08-17
- Completed: {YYYY-MM-DD}
- Branch: feature/update-sora-cpp-sdk-2026.2.0
- Polished: 2026-08-19

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

今回の追従で sora-unity-sdk 側のコード変更が必要なのは TLS 検証のシステム CA 化への対応のみである。DEPS の各バージョン更新は値の書き換えのみで、ソースコードへの影響は以下の理由によりない。

- `WEBRTC_BUILD_VERSION` の `m150.7871.3.0` → `m150.7871.3.1` は libwebrtc 本体のコミット (WEBRTC_COMMIT) が同一のため、webrtc ヘッダ API に影響しない
- `BOOST_VERSION` / `CMAKE_VERSION` の更新は sora-unity-sdk が取得するプリビルドパッケージのバージョン参照のみで、sora-unity-sdk のソースコードには影響しない
- `SoraClientContext` の ABI 変更 (MediaEngineReference 保持) と `BOOST_ASIO_ENABLE_VERSION_NAMESPACE` の有効化は、いずれも現依存の 2026.2.0-canary.18 に既に含まれており、今回の追従で追加対応は不要である

なお、sora-cpp-sdk 2026.2.0 では NVIDIA Pascal 世代以前の GPU サポートが廃止された。sora-unity-sdk のコード変更は不要だが、`VideoCodecImplementation.NvidiaVideoCodec` を利用する GTX 10 シリーズでハードウェアエンコーダー / デコーダーが使えなくなる。依存追従に伴う利用者への影響として、`CHANGES.md` の [UPDATE] エントリ内で周知する。

### TLS 検証のシステム CA 化への対応

sora-cpp-sdk 2026.2.0 で TLS 検証の信頼ストアが OS のシステム CA に切り替わった。sora-unity-sdk 側の変更は次の 1 点のみ。

- `SoraUnitySdkPostProcessor.cs` の `AddFrameworkToProject` に `Security.framework` を追加する
  - 背景: iOS は `SecTrustEvaluateWithError` による検証委譲のため、iOS アプリのビルドに `Security.framework` の追加が必要になる
  - 独自 CA を使う場合は `Config.CACert` (ca_cert) に PEM を明示指定する（既存機能で対応済み）
  - 対象バージョン: iOS は iOS 14 以降、macOS は Sonoma 14.x 以降が対象（sora-cpp-sdk 2026.2.0 の仕様）
  - sora-unity-sdk の iOS ビルドは deployment target 13.0（run.py の `CMAKE_OSX_DEPLOYMENT_TARGET`）のため、iOS 13 端末での TLS 検証の挙動を確認する。iOS 13 で TLS 検証が機能しない場合は対応方針を決定する（別 issue での対応も可）

## 完了条件

- 全プラットフォーム (windows_x86_64 / macos_arm64 / ubuntu-22.04_x86_64 / ubuntu-24.04_x86_64 / ios / android) でビルドが成功する
- SoraUnitySdkExamples の iOS ビルドで生成される Xcode プロジェクトに `Security.framework` が追加される
  - CI は `run.py build` によるライブラリビルドのみで `SoraUnitySdkPostProcessor.cs` が実行されないため、Unity での iOS ビルドによる確認が必要
- iOS 13 端末での TLS 検証の挙動を確認する
- `CHANGES.md` の develop の [UPDATE] エントリを 2026.2.0 向けに更新する
  - TLS システム CA 化に伴う `Security.framework` の追加を含める
  - TLS 検証の挙動変更と、独自 CA を使う場合は `Config.CACert` (ca_cert) に PEM を指定する旨を含める
  - NVIDIA Pascal 世代以前の GPU サポート廃止に伴う影響を含める

## 解決方法

1. DEPS を更新する
2. `SoraUnitySdkPostProcessor.cs` に `Security.framework` を追加する
3. 全プラットフォームでビルドを検証する
4. SoraUnitySdkExamples を iOS ビルドし、生成された Xcode プロジェクトに `Security.framework` が含まれることを確認する
5. iOS 13 端末で TLS 検証の挙動を確認する
6. `CHANGES.md` の develop の [UPDATE] エントリを 2026.2.0 向けに更新する
