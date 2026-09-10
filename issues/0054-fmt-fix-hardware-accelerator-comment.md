# GetHardwareAcceleratorPreference のコメントを実装と一致させる

- Priority: Medium
- Created: 2026-08-27
- Branch: fmt/fix-hardware-accelerator-comment
- Polished: 2026-09-10

## 目的

`SoraUnitySdkExamples/Assets/SoraUnitySdk/Sora.cs` の `GetHardwareAcceleratorPreference` に付いているコメントの優先順位記述が実装と食い違っているため、実装に合わせて修正する。

## 現状

`Sora.cs` の `GetHardwareAcceleratorPreference` のコメントには「優先度的には Intel VPL > AMD AMF > Nvidia Video Codec > Internal」と書かれている。

しかし実装では AMD AMF の `Merge` がコメントアウトされており、実質的には `Internal → NvidiaVideoCodec → IntelVpl` の順で `Merge` されている。

CHANGES.md 2025.3.0 では AMD AMF ハードウェアアクセラレーターを非推奨化し、優先リストから除外したことが明記されている（「AMD AMF ハードウェアアクセラレーターを非推奨化する」「AMD AMF は現在非推奨であるため、ハードウェアエンコーダー・デコーダーの優先リストから除外する」。同エントリには「将来的にはまた利用可能にするため、コードは残してコメントアウト」ともある）。コメントだけが古い順序のまま残っており、読み手は「コメント通りの順に反映される」と誤解する。

なお、0065（`Sora.cs` の AMD AMF コメントアウトブロックと VideoTrack TODO を削除する）が AMD AMF の `Merge` コメントアウトブロックの削除を担当しており、0065 は「`GetHardwareAcceleratorPreference` のコメント修正は別 issue で対応する」としている。0065 が先に実装された場合は `Sora.cs` から AMD AMF の `Merge` コメントアウトブロックは消えるが、本 issue の対象となる優先順位コメントは残る。

## 設計方針

- `GetHardwareAcceleratorPreference` のコメントを、実装の順序と一致するよう書き換える
- 「AMD AMF は非推奨のため対象外」を明記する
- `Merge` の呼び出し順とコメントを 1 対 1 で対応させる
- コメントは現行どおり優先度の高い順（`Intel VPL > Nvidia Video Codec > Internal`）で記述する。実装の `Merge` は `Internal → NvidiaVideoCodec → IntelVpl` の順（優先度の低い順）であり、関数内の既存コメント（同じコーデックは後からマージした方が優先される）と表裏の関係になる

## 完了条件

- `GetHardwareAcceleratorPreference` のコメントに書かれている優先順位が「Intel VPL > Nvidia Video Codec > Internal」となっており、実装の `Merge` 結果と一致している
- AMD AMF が対象外である理由が明記されている
- コード側の挙動は変更されていない（コメントのみ修正）
