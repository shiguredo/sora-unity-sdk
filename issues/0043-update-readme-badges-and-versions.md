# README の libwebrtc バッジ・Copyright・対応 Sora バージョンを更新する

- Priority: High
- Created: 2026-08-27
- Branch: update/readme-badges-and-versions
- Polished: {YYYY-MM-DD}
- Milestone: 2026.2.0

## 目的

`README.md` の libwebrtc バッジ、Copyright、対応 Sora バージョンが古い記載のままになっている。develop の実態および `DEPS` と一致させる。

## 現状

`README.md` に次の古い記載が残っている。

- libwebrtc バッジ: `m144.7559` を指しているが、`DEPS` は `WEBRTC_BUILD_VERSION=m150.7871.3.1`
- Copyright: `Copyright 2019-2025, Wandbox LLC` と `Copyright 2019-2025, Shiguredo Inc.` のまま。2026 年のリリースに向けて 2026 に更新すべき
- 対応 Sora バージョン: `WebRTC SFU Sora 2025.1.0 以降` と記載されているが、CHANGES.md 2026.1.0 で追加した `RPC 機能` と `simulcast_request_rid` は Sora 2025.2 以降でのみ利用可能。実質必要な Sora バージョンは 2025.2.0 以降

## 設計方針

- libwebrtc バッジの URL とラベルを `m150.7871` および branch-heads/7871 に更新する
- Copyright を `2019-2026` に更新する
- 対応 Sora バージョンの記述を `WebRTC SFU Sora 2025.2.0 以降` に修正する
- `SoraUnitySdkExamples/README.md` にも同様の記載があれば揃える

## 完了条件

- libwebrtc バッジが `m150.7871` を指している
- Copyright が `2019-2026` になっている
- 対応 Sora バージョンが `2025.2.0 以降` になっている
- `SoraUnitySdkExamples/README.md` も揃っている
