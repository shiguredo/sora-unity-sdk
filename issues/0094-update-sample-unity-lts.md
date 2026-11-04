# サンプルプロジェクトを Unity 6.3 LTS の最新パッチへ更新し動作確認する

- Created: 2026-11-04
- Completed: {YYYY-MM-DD}
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
  Scenes In Build は空であり、macOS Player をビルドするには
  `multi_sendrecv.unity` を追加する必要がある
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
- macOS Player のビルドでは、`SoraUnitySdkExamples/ProjectSettings/EditorBuildSettings.asset`
  の Scenes In Build に `Assets/Scenes/multi_sendrecv.unity` を追加する。
  この変更はサンプルプロジェクトに含める
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
