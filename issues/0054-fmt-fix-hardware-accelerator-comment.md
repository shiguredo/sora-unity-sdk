# GetHardwareAcceleratorPreference のコメントを実装と一致させる

- Priority: Medium
- Created: 2026-08-27
- Branch: fmt/fix-hardware-accelerator-comment
- Polished: {YYYY-MM-DD}

## 目的

`SoraUnitySdkExamples/Assets/SoraUnitySdk/Sora.cs` の `GetHardwareAcceleratorPreference` に付いているコメントの優先順位記述が実装と食い違っているため、実装に合わせて修正する。

## 現状

`Sora.cs` の `GetHardwareAcceleratorPreference` のコメントには「優先度的には Intel VPL > AMD AMF > Nvidia Video Codec > Internal」と書かれている。

しかし実装では AMD AMF の `Merge` がコメントアウトされており、実質的には `Internal → NvidiaVideoCodec → IntelVpl` の順で `Merge` されている。

CHANGES.md 2025.3.0 で「AMD AMF ハードウェアアクセラレーターを非推奨化する。優先リストから除外」と明記済みだが、コメントだけが古い順序のまま残っている。読み手は「コメント通りの順に反映される」と誤解する。

## 設計方針

- `GetHardwareAcceleratorPreference` のコメントを、実装の順序と一致するよう書き換える
- 「AMD AMF は非推奨のため対象外」を明記する
- `Merge` の呼び出し順とコメントを 1 対 1 で対応させる

## 完了条件

- `GetHardwareAcceleratorPreference` のコメントに書かれている順序が実装の `Merge` 順と一致している
- AMD AMF が対象外である理由が明記されている
- コード側の挙動は変更されていない（コメントのみ修正）
