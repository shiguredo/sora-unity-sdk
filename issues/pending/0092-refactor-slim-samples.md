# サンプルをシーンごとに分割してスリム化する

- Created: 2026-09-11
- Completed: {YYYY-MM-DD}
- Branch: feature/refactor-slim-samples
- Polished: {YYYY-MM-DD}

## 目的

Sora Unity SDK のサンプルは `SoraSample.cs` 1 ファイルに sendrecv / sendonly / recvonly の機能がまとまっており、機能も多い。最低限動くサンプルとして構成を見直し、メンテナンスコストを下げる。

## 現状

- `SoraUnitySdkExamples/Assets/SoraSample.cs` は約 1355 行で、`SampleType` の MultiSendrecv / MultiRecvonly / MultiSendonly を 1 クラスで分岐している。
- シーンは `SoraUnitySdkExamples/Assets/Scenes/multi_sendrecv.unity` / `multi_recvonly.unity` / `multi_sendonly.unity` の 3 つがある。
- 設定項目やボタンが多く、利用したい機能をどう組み込むかの参考にしづらい。

## 設計方針

- 現在のサンプルから機能を削り、最低限の接続・切断と送受信が分かる構成にする。
- sendrecv / sendonly / recvonly はシーンとして維持する。
- `SoraSample.cs` をシーンごとに分割し、それぞれのシーン専用のスクリプトを用意する。
- 詳細な設定を試したい場合の導線は別途検討する。

## 完了条件

- シーンごとにスクリプトが分割され、1 スクリプトが担う機能が明確になっている。
- 各シーンが必要最小限の機能で動作する。
- サンプルのメンテナンスコストが下がっている。

## pending にした理由

2026-09-11 に pending にする。

- サンプルから何を残し何を削るか、シーンごとのスクリプト構成をどうするかの設計判断が必要である。
- サンプルの構成は README やドキュメントからの導線にも影響するため、ドキュメント側の方針と合わせて決める必要がある。
- 方針が決まるまでは既存サンプルの分割に着手できない。
