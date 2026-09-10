# Unity 側で生成した音声/映像ストリームを SDK に渡す口を作る

- Created: 2026-09-10
- Completed: {YYYY-MM-DD}
- Branch: feature/add-unity-stream-input
- Polished: {YYYY-MM-DD}

## 目的

Unity アプリ側で生成した映像や音声を、SDK のカメラ / マイクデバイスの経路とは別に、アプリから直接 SDK へ渡せるようにする。Unity のカメラ制御やネイティブレンダリングの口をプラットフォーム側の事情で SDK から直接扱えない環境でも、アプリが用意した映像を送信できるようにするため。

## 現状

- 映像入力はネイティブ側の `UnityCameraCapturer` が Unity カメラの描画結果を取得して `VideoTrack` に流している。C# 側から任意の映像フレームを投入する API はない。
- `src/sora.cpp` の `Sora::SetOnCapturerFrame` と `Sora::RenderCallback` は SDK から Unity への描画・コールバック方向の経路であり、Unity から SDK へ映像を渡す口ではない。
- 音声入力は `Sora.Config.UnityAudioInput` を true にして `Sora.ProcessAudio(float[], int, int)` を呼ぶと、録音デバイスの代わりにアプリが渡した 48000Hz ステレオ float データを送信音声として扱える。Unity から SDK へ音声を渡す口は既にある。
- そのため、Unity 側で生成した映像を SDK へ渡す経路が未整備である。

## 設計方針

- C# 側から任意の映像フレーム (テクスチャ / ネイティブテクスチャ / 生ピクセル) を SDK に投入する API を検討する。既存の `UnityCameraCapturer` を置き換えるのではなく、入力元を選べる形にする。
- `Sora.Config` に Unity 映像入力の有効 / 無効を追加し、既存のカメラキャプチャ経路と排他にするか併用可能にするかを決める。
- 受け取る映像フォーマット (RGBA / I420 / ネイティブテクスチャ)、解像度、フレームレート、タイムスタンプの扱いを決める。
- 音声は既存の `Sora.Config.UnityAudioInput` と `Sora.ProcessAudio()` を利用する。音声側に不足がある場合 (サンプルレートやチャンネルが固定など) は別 issue に分ける。
- Windows / macOS / Android / iOS の各ターゲットで扱えるようにする。
- 既存の Unity カメラキャプチャ経路の挙動は変えない。

## 完了条件

- Unity C# から任意の映像フレームを SDK に渡し、Sora へ送信できる
- `Sora.Config` の既存カメラ設定・デバイス設定の挙動を変えない
- Windows / macOS / Android / iOS の各ビルドが通る
- `CHANGES.md` の `## develop` に `[ADD]` エントリを追記する

## pending にした理由

2026-09-10 に pending にする。

- 入力する映像の形式 (テクスチャ種別・ピクセルフォーマット) と、C# からネイティブへ受け渡す API の設計が未確定である。
- 既存の `UnityCameraCapturer` との排他 / 併用、解像度・フレームレート・タイムスタンプの扱いを決める必要がある。
- 受理できる映像ソースの範囲が広く、これらの設計が決まるまで実装方針を確定できないため保留とする。対応を再開するときは reopened にしてから、API と映像形式を確定する。
