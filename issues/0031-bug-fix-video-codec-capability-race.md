# g_video_codec_capability の static グローバル状態をスレッドセーフにする

- Priority: High
- Created: 2026-08-27
- Branch: fix/video-codec-capability-race
- Polished: {YYYY-MM-DD}
- Milestone: 2026.2.0

## 目的

`src/unity.cpp` の VideoCodecCapability 取得 API が非スレッドセーフなグローバル state に依存している経路を解消する。複数 Sora インスタンスや複数スレッドから並行して呼ばれると内部の shared_ptr refcount が race し、二重 free の可能性がある。

## 現状

`src/unity.cpp` に `static std::optional<sora::VideoCodecCapability> g_video_codec_capability;` がファイルスコープで置かれている。以下の 2 段階 API が同じグローバルを共有する。

- `sora_get_video_codec_capability_size`: capability を作成して `g_video_codec_capability` に代入し、JSON サイズを返す
- `sora_get_video_codec_capability`: 上記グローバルから JSON を取り出して buf に書き、`reset()` する

問題点:

- 両者の間に mutex が無く、`std::optional` のセットとリードには何のロックもない
- `sora::GetVideoCodecCapability` は内部で NVML や CUDA / VPL の初期化を行うため、複数スレッドから叩かれると初期化と参照カウント操作が race する
- グローバル state のため複数 Sora インスタンスから叩かれるとサイズと本体が混ざる
- C ABI として任意スレッドから呼ばれる契約になっている

## 設計方針

- `g_video_codec_capability` を保護する `std::mutex` を導入し、size / read / reset の各操作を lock 内で完結させる
- あるいは 2 段階 API を廃し、size + data を 1 コールで返す新 API を提供して C# 側で置き換える
- 短期的には mutex 導入で最小の変更にとどめ、長期的には API 分離を検討する

## 完了条件

- `sora_get_video_codec_capability_size` と `sora_get_video_codec_capability` の間で内部状態が race しない
- 複数スレッド、複数 Sora インスタンスから並行して呼ばれても正しい結果が返る
- `CHANGES.md` の `## develop` に `[FIX]` を追記する
