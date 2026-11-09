# サンプルプロジェクトを Unity 6.3 LTS の最新パッチへ更新し動作確認する

- Created: 2026-11-04
- Completed: 2026-11-05
- Branch: feature/update-sample-unity-lts
- Polished: 2026-11-04

## 目的

サンプルプロジェクトを Unity 6.3 LTS の最新安定パッチへ更新し、
セキュリティ修正を取り込む。Unity 6.0 LTS の対応も継続する。

Unity のパッチ更新に伴う確認として、Unity と直接連携する箇所を
代表環境で検証する。

## 現状

- `SoraUnitySdkExamples/ProjectSettings/ProjectVersion.txt` の
  `m_EditorVersion` は `6000.3.0f1`
- `SoraUnitySdkExamples/ProjectSettings/EditorBuildSettings.asset` の
  Scenes In Build は空であり、ビルド対象が空の場合は
  開いているシーンがビルドされる
- `README.md` と `SoraUnitySdkExamples/README.md` は
  Unity 6000.3 / 6000.0 の両系列を対応対象としている
- `CHANGES.md` の `## develop` には、サンプルプロジェクトの
  Unity バージョン更新のエントリがない
- `src/sora.cpp` の `Sora::CreateVideoCapturer()` と
  `Sora::CreateADM()` は、通常のデバイスカメラ取得と音声入出力を
  Sora C++ SDK 側へ委譲している
- Unity との直接の接点として、ネイティブ関数呼び出しに加え、
  以下の処理がある
  - `Sora.cs` の `Connect()` と `OnRender()` による
    Unity Camera のテクスチャ取得とレンダリングイベントの発行
  - `Sora.cs` の `RenderTrackToTexture()` による映像の描画
  - `SoraSample.cs` の `Render()` による Unity 音声の取り込み
  - `SoraSample.cs` の `InitSora()` による
    `AudioClip` と `AudioSource` を使った受信音声の再生

## 設計方針

- 着手時に Unity 公式の配布情報とセキュリティ勧告を確認し、
  6.3 系と 6.0 系それぞれの最新安定パッチを確定する
- サンプルプロジェクトを、確定した 6.3 系のパッチで開く
- `ProjectVersion.txt` の `m_EditorVersion` と
  `m_EditorVersionWithRevision` を更新し、Unity が生成する変更は
  更新に必要なものだけを反映する
- 6.0 系の確認には、既存の `SoraUnitySdkExamples` を複製した
  一時プロジェクトを用意し、同じ SDK とサンプルを使用する。
  リポジトリには 6.0 用プロジェクトを追加しない
- SDK 本体、依存ライブラリ、描画設定の変更は前提としない
- プラグインフォルダの整理は [公式導入手順](https://sora-unity-sdk.shiguredo.jp/use#3bca53)
  に従って利用者が手動で行う運用を維持し、`.meta` の対象 OS 設定は変更しない
- Scenes In Build は空の状態を維持し、利用者が開いた
  sendonly / recvonly などのシーンをビルドする既存の使い方を保つ
- macOS Player の検証では `multi_sendrecv` シーンを開くか、
  検証用のビルド処理で対象シーンを明示する。
  検証用のシーン指定をサンプルプロジェクトの既定設定として保存しない
- 更新に伴ってコードやプラットフォーム固有設定の修正が必要になった場合は、
  その変更箇所に対応する検証を追加する

## 検証内容

### 共通条件

- 代表環境は macOS arm64 / Metal とする
- 使用するシーンは
  `SoraUnitySdkExamples/Assets/Scenes/multi_sendrecv.unity` のみとする
- 実際の Sora サーバーと別の接続相手を使用し、
  モックやスタブは使わない
- 映像は相手への送信と相手からの受信描画、
  音声は相手への送信と相手からの受信再生を確認する

### 6.3 系の最新安定パッチ / Editor

- インポートとコンパイルが成功する
- デバイスカメラとマイクを使って接続し、映像と音声を送受信できる
- 接続中に Unity Camera へ切り替え、相手側の映像が更新される
- 切断して再接続できる
- Play モードを終了してもクラッシュやフリーズが発生しない

Unity 経由の音声入出力は、この環境でのみ追加確認する。

- `unityAudioInput` と `unityAudioOutput` を有効にして接続し直す
- `SoraSample.Render()` の条件に合わせ、音声出力をステレオに設定する
- Unity で再生した音が相手に届き、相手からの音声を
  `AudioSource` 経由で再生できる

### 6.3 系の最新安定パッチ / macOS Player

- サンプルをビルドして起動できる
- デバイスカメラとマイクを使って映像と音声を送受信できる
- 正常に切断できる

### 6.0 系の最新安定パッチ / Editor

- 検証用プロジェクトでインポートとコンパイルが成功する
- デバイスカメラとマイクを使って映像と音声を送受信できる
- 正常に切断できる

### 結果の記録

使用した Unity のパッチ番号、SDK のコミット、OS、
描画 API、確認項目と結果を、本 issue の解決方法に記録する。
検証済みとする範囲は、実際に確認した環境と項目に限定する。

## 範囲外

- 全 OS、全グラフィックス API、全コーデックの網羅
- sendonly / recvonly シーンの個別確認
- サイマルキャスト、RPC、詳細なデバイス操作の再検証
- ネイティブライブラリの全ターゲットでの再ビルド
- 長時間運転と性能測定
- CI の検証構成の拡張

## 完了条件

- サンプルプロジェクトが、着手時に確定した
  Unity 6.3 LTS の最新安定パッチへ更新されている
- 上記 3 組み合わせの検証と、6.3 系 Editor での
  Unity 経由の音声入出力の確認が完了している
- 更新に起因する問題が解消され、使用したパッチ番号と結果が記録されている
- `CHANGES.md` の `## develop` の `### misc` に、
  更新後の Unity バージョンを記載した `[UPDATE]` エントリを追記する
- README とサンプルの README で Unity 6.0 の対応が維持されている
- Scenes In Build が空のままで、ビルド対象が特定のサンプルに固定されていない

## 解決方法

### 更新内容

- Unity 公式の配布 API とセキュリティ勧告を確認し、着手時の最新安定パッチを確定した
  - 6.3 LTS: `6000.3.28f1`、リビジョン `8f082d2fe476`
  - 6.0 LTS: `6000.0.85f2`、リビジョン `b028303d8e7d`
  - 両パッチは 2026-11-04 (JST) に配布を確認した
  - [Unity 6.3 の公式配布情報](https://services.api.unity.com/unity/editor/release/v1/releases?version=6000.3&limit=3&order=RELEASE_DATE_DESC)
  - [Unity 6.0 の公式配布情報](https://services.api.unity.com/unity/editor/release/v1/releases?version=6000.0&limit=3&order=RELEASE_DATE_DESC)
  - [Brotli のセキュリティ勧告](https://unity.com/security/unity-brotli-decoder-vulnerability)
  - [eMudhra のセキュリティ勧告](https://unity.com/security/unity-emudhra-compromise)
- `m_EditorVersion` と `m_EditorVersionWithRevision` を 6.3 LTS の確定値へ更新した
- Scenes In Build は空のままとし、ビルド対象を特定のサンプルに固定しない構成を維持した
- 変更履歴の `develop` / `misc` に Unity `6000.3.28f1` への更新を追記した
- 6.0 系で提供されない未使用の組み込みモジュール
  `com.unity.modules.adaptiveperformance` と `com.unity.modules.vectorgraphics` の参照を削除した
- 両 README の Unity 6000.3 / 6000.0 の対応表記を維持した

### 検証環境と準備

- OS: macOS `27.0.1` (ビルド `26A434`)、arm64
- 描画 API: Metal。Editor と Player の実行時情報でも確認した
- SDK ソースのコミット: `7fff2388c0dc66713682f37487fc3bf3cc73bf33`
- 検証用 SDK 配布版: `2026.2.0-canary.7`
  - ビルド元のコミット: `68ac77a4b55c7d83307439caf5fa5edfc1713dc8`
  - SDK の C# / ネイティブ実装、プロトコル定義、依存バージョンは、上記ソースのコミットと同一であることを確認した
- 確定した両 Unity パッチの arm64 版をインストールした
- 6.0 系の確認用に、同じ SDK とサンプルを含む一時プロジェクトを用意した
- ローカル Sora の起動と全ノードの正常性を確認した
- ライセンスの更新と Rosetta 2 の導入により、両 Editor の起動時の障害を解消した

### 検証結果

| 項目 | 結果 |
| --- | --- |
| Unity バージョンとリビジョンの公式配布情報との一致 | 確認済み |
| Scenes In Build の空の既定設定の維持 | `m_Scenes: []` を確認 |
| Ubuntu 用プラグインのメタデータの維持 | 2 つの `libSoraUnitySdk.so.meta` が変更前と一致することを確認 |
| 両 README の Unity 6.0 対応表記の維持 | 確認済み |
| 差分の空白エラー | `git diff --check` 成功 |
| 6.3 Editor のインポートとコンパイル | 成功。既存 API の非推奨警告あり |
| 6.3 Editor のデバイスカメラの映像送信と受信描画 | 確認済み |
| 6.3 Editor の Unity Camera への切替と相手側の映像更新 | 確認済み |
| 6.3 Editor の切断・再接続と Play モード終了 | 確認済み |
| 6.3 Editor の Unity 経由の音声入出力 | ステレオで確認済み。相手の音声受信統計と AudioSource の非ゼロ再生データを確認 |
| 6.3 macOS arm64 Player のビルドと起動 | 成功 |
| 6.3 macOS Player の映像送受信と音声受信 | 確認済み。受信音の聴取も確認 |
| 6.3 macOS Player の切断と終了 | `CLOSE_SUCCEEDED` と終了コード 0 を確認 |
| 6.0 Editor のインポートとコンパイル | 成功 |
| 6.0 Editor の映像送受信と音声受信 | 確認済み |
| 6.0 Editor の切断と終了 | `CLOSE_SUCCEEDED` と終了コード 0 を確認 |
| デバイスマイクからの音声送信 | 3 環境とも、マイク入力と相手側の非ゼロの受信音声統計を確認 |

Unity 経由の音声確認では、AudioSource の再生処理で 2,117,060 サンプル以上の
非ゼロデータを記録した。相手側でも非ゼロの受信音声を確認した。
映像受信は RTP の受信・復号統計だけでなく、描画後の GPU テクスチャを読み戻して確認した。

デバイスマイクの検証では、実行ファイルを直接起動した際に macOS の公開 API が
`AVAuthorizationStatusDenied` を返し、送信音声が無音となった。
通常のアプリ起動経路に切り替えて macOS のマイクアクセスを許可すると解消した。
その状態で Player と両 Editor の送受信を確認した。

検証専用コードをリポジトリから除去した状態でも、`multi_sendrecv` を対象とする
macOS arm64 Player のビルドが成功することを確認した。
ビルド対象の指定は検証時に限定し、Scenes In Build は空の既定設定に戻した。
ビルド検証時は、Ubuntu 用プラグインの対象 OS を限定して同名ファイルの重複を除外した。
この `.meta` の変更は取り下げ、最終構成では公式導入手順に従う手動のフォルダ整理を前提とする。
Scenes In Build と `.meta` を既定設定に戻した後の Player ビルドは再実行していない。
SDK 本体と依存ライブラリのバージョンは変更していない。
検証済みの範囲は、上記の Unity パッチと macOS arm64 / Metal の組み合わせに限定する。
