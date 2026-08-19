# Sora C++ SDK を 2026.2.1 に上げる

- Created: 2026-08-17
- Completed: {YYYY-MM-DD}
- Branch: feature/update-sora-cpp-sdk-2026.2.1
- Polished: 2026-08-19

## 目的

sora-unity-sdk のネイティブ部分は sora-cpp-sdk を利用しており、そのバージョンはリポジトリ直下の `DEPS` ファイルで管理している。sora-cpp-sdk 2026.2.1 が 2026-08-18 にリリースされたため、依存を最新リリースに追従する。

## 現状

`DEPS` は `SORA_CPP_SDK_VERSION=2026.2.0-canary.18` を参照しており、canary.18 から 2026.2.1 までの差分が未反映である。

sora-cpp-sdk 2026.2.1 は 2026.2.0 のパッチリリースであり、追加差分は次の 1 件のみ。

- [FIX] 切断処理のタイマーが解放済みの WebSocket に対して `Cancel()` を呼び SIGSEGV でクラッシュするのを修正する
  - DataChannel シグナリング利用時の切断で、WebSocket close の完了が DataChannel の close 通知より先に処理されるとクラッシュしていた

## 設計方針

`DEPS` の各バージョンを 2026.2.1 に合わせて更新する。今回の追従で sora-unity-sdk 側のコード変更が必要なのは TLS 検証のシステム CA 化への対応のみである。

- `SORA_CPP_SDK_VERSION`: `2026.2.0-canary.18` → `2026.2.1`
- `WEBRTC_BUILD_VERSION`: `m150.7871.3.0` → `m150.7871.3.1`
- `BOOST_VERSION`: `1.91.0` → `1.92.0`
- `CMAKE_VERSION`: `4.3.2` → `4.4.2`

DEPS の各バージョン更新は値の書き換えのみで、ソースコードへの影響は以下の理由によりない。

- `WEBRTC_BUILD_VERSION` の `m150.7871.3.0` → `m150.7871.3.1` は libwebrtc 本体のコミット (WEBRTC_COMMIT) が同一のため、webrtc ヘッダ API に影響しない
- `BOOST_VERSION` / `CMAKE_VERSION` の更新は sora-unity-sdk が取得するプリビルドパッケージのバージョン参照のみで、sora-unity-sdk のソースコードには影響しない
- `SoraClientContext` の ABI 変更 (MediaEngineReference 保持) と `BOOST_ASIO_ENABLE_VERSION_NAMESPACE` の有効化は、いずれも現依存の 2026.2.0-canary.18 に既に含まれており、今回の追従で追加対応は不要である
- 2026.2.1 の WebSocket クラッシュ修正は sora-cpp-sdk 内部の修正であり、sora-unity-sdk 側のコード変更は不要である（sora-unity-sdk が sora-cpp-sdk のクラッシュ修正を利用できるようになる）

### TLS 検証のシステム CA 化への対応

sora-cpp-sdk 2026.2.0 で TLS 検証の信頼ストアが OS のシステム CA に切り替わった。sora-unity-sdk 側の変更は次の 1 点のみ。

- `SoraUnitySdkExamples/Assets/SoraUnitySdk/Editor/SoraUnitySdkPostProcessor.cs` の `OnPostprocessBuild` 内で、既存の `AddFrameworkToProject` 呼び出し（`VideoToolbox.framework` / `GLKit.framework` / `Network.framework`）に `Security.framework` を追加する
  - 背景: iOS は sandbox 制約により `SecTrustEvaluateWithError` に検証を委譲する方式で実装されているため、iOS アプリのビルドに `Security.framework` の追加が必要になる
  - 独自 CA を使う場合は `Config.CACert` (ca_cert) に PEM を明示指定する（既存機能で対応済み）

### 利用者への影響

sora-cpp-sdk 2026.2.0 では NVIDIA Pascal 世代以前の GPU サポートが廃止された。sora-unity-sdk のコード変更は不要だが、`VideoCodecImplementation.NvidiaVideoCodec` を利用する GTX 10 シリーズではハードウェアエンコーダー / デコーダーが使えなくなる。依存追従に伴う利用者への影響として、`CHANGES.md` の [UPDATE] エントリ内で周知する。

## 完了条件

- 全プラットフォーム (windows_x86_64 / macos_arm64 / ubuntu-22.04_x86_64 / ubuntu-24.04_x86_64 / ios / android) でビルドが成功する
  - CI (`build.yml`) の `python3 run.py build <target>` が全ターゲットで成功する
- SoraUnitySdkExamples の iOS ビルドで生成される Xcode プロジェクトに `Security.framework` が追加される
  - CI は `run.py build` によるライブラリビルドのみで `SoraUnitySdkPostProcessor.cs` が実行されないため、Unity での iOS ビルドによる確認が必要
- `CHANGES.md` の develop の [UPDATE] エントリを 2026.2.1 向けに更新する
  - 既存の `2026.2.0-canary.18` エントリを書き換える
  - 2026.2.1 で修正されたクラッシュを [UPDATE] エントリ内のサブ項目として追記する
  - 記載内容は後述の「変更履歴」のサンプルのとおり

## 変更履歴

`CHANGES.md` の develop の [UPDATE] エントリを次のとおり更新する。`streams()` 修正は canary.18 で実施済みのためエントリに残す。

```md
- [UPDATE] Sora C++ SDK を `2026.2.1` に上げる
  - libwebrtc を `m150.7871.3.1` に上げる
  - BOOST_VERSION を `1.92.0` にアップデート
  - CMAKE_VERSION を `4.4.2` にアップデート
  - libwebrtc m150 で `stream_ids()` が削除されたため、 `streams()` を使うように修正する
  - TLS 検証の信頼ストアを OS のシステム CA に切り替える
  - iOS の TLS 検証のシステム CA 化に伴い、ビルド時に `Security.framework` を追加する
  - 独自 CA を使う場合は `Config.CACert` (ca_cert) に PEM を指定する
  - NVIDIA Pascal 世代以前の GPU サポートが廃止されたため、GTX 10 シリーズではハードウェアエンコーダー / デコーダーが使えなくなる
  - DataChannel シグナリング利用時の切断で、WebSocket close の完了が DataChannel の close 通知より先に処理されるとクラッシュする問題を修正する
  - @対応者
```

## 解決方法

未着手 (PR 作成後に追記する)
