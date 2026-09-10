# Sora Unity SDK サンプルのビットレート設定を Sora DevTools と揃える

- Created: 2026-09-10
- Completed: {YYYY-MM-DD}
- Branch: feature/change-align-sample-bitrate-options-with-sora-devtools
- Polished: {YYYY-MM-DD}

## 目的

Sora Unity SDK のサンプルではビットレートの選択肢が用意されておらず、Sora DevTools と同じ条件でビットレートを検証できない。Sora DevTools と同じ選択肢で映像・音声ビットレートを指定できるようにし、他の SDK のサンプルと検証条件を揃える。

## 現状

`SoraUnitySdkExamples/Assets/SoraSample.cs` には `public int videoBitRate = 0;` があるだけで、Sora DevTools のような選択肢はなく、Unity Inspector で任意の整数を入力する運用になっている。`SoraSample.cs` には音声ビットレートのフィールド自体がなく、`Sora.cs` の `SoraAudioOption.AudioBitRate` はサンプルから設定されていない。そのため Unity サンプルでは音声ビットレートを指定できない。

シーンの `videoBitRate` は `multi_sendrecv.unity`、`multi_sendonly.unity`、`multi_recvonly.unity` のいずれも `0` のままで、ビットレート未指定の状態になっている。

Sora DevTools (`sora-devtools` の `src/constants.ts`) の選択肢は次のとおり。

- `VIDEO_BIT_RATES`: 未指定, `10`, `30`, `50`, `100`, `300`, `500`, `800`, `1000`, `1500`, `2000`, `2500`, `3000`, `5000`, `10000`, `15000`, `20000`, `30000`, `50000`
- `AUDIO_BIT_RATES`: 未指定, `8`, `16`, `24`, `32`, `64`, `96`, `128`, `256`, `384`

## 設計方針

- サンプルで映像・音声ビットレートを Sora DevTools と同じ選択肢から選べるようにする。
- Inspector で選択する項目なので、`videoSize` と同様の enum にするか、`int` フィールドのまま選択肢を別途用意するかは実装時に判断する。
- 映像と音声を別々に指定できるようにする。
- 未指定 (`0`) の場合はビットレートを指定しない既存の挙動を維持する。

## 完了条件

- Unity サンプルで映像・音声ビットレートを Sora DevTools と同じ選択肢から指定できる。
- 未指定 (`0`) の既存の挙動が維持されている。
- シーンの既存設定が意図せず変更されていない。
