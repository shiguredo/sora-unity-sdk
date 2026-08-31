# g_video_codec_capability の static グローバル状態をスレッドセーフにする

- Priority: High
- Created: 2026-08-27
- Branch: feature/fix-video-codec-capability-race
- Polished: 2026-08-31
- Milestone: 2026.2.0

## 目的

`src/unity.cpp` の VideoCodecCapability 取得 API が非スレッドセーフなグローバル state に依存している経路を解消する。現状は 2 段階 API がグローバル変数を共有しており、複数 Sora インスタンスや複数スレッドから並行して呼ばれると data race (未定義動作) が起き、結果の混線や二重 free の可能性がある。

## 現状

`src/unity.cpp` に `static std::optional<sora::VideoCodecCapability> g_video_codec_capability;` がファイルスコープで置かれている。以下の 2 段階 API が同じグローバルを共有する。

- `sora_get_video_codec_capability_size`: capability を作成して `g_video_codec_capability` に代入し、JSON サイズを返す
- `sora_get_video_codec_capability`: `g_video_codec_capability` がセットされていれば config 引数を無視してグローバルから JSON を取り出して buf に書き、`reset()` する。無ければ config から再計算する

問題点:

- 両者の間に mutex が無く、`std::optional` と内部コンテナ (`std::vector`) への並行アクセスにロックがない
- 2 段階呼び出しのペアは C# 側 `Sora.GetVideoCodecCapability` (static メソッド) の `size` 呼び出しと `data` 呼び出しで構成される。別々の C ABI 呼び出しのため「A が size → B が size → A が data」と横取りされると、A は B の capability を受け取る (data 側はグローバルがあれば config を照合せず流用するため)
- `sora::GetVideoCodecCapability` は内部で CUDA / VPL / AMF の初期化を行う (sora-cpp-sdk の `src/sora_video_codec.cpp` の `GetVideoCodecCapability` が NVIDIA / Intel / AMD 各 session を生成する)。並行呼び出しで初期化と破棄が race する
- `g_video_codec_capability` は Sora インスタンスと無関係なファイルスコープ static のため、複数 Sora インスタンスから叩かれるとサイズと本体が混ざる
- C# 側は static P/Invoke でスレッド制約が明示されておらず、任意スレッドから呼び得る

## 設計方針

- 2 段階 API (`sora_get_video_codec_capability_size` / `sora_get_video_codec_capability`) を廃止し、**1 コールで JSON サイズとデータの両方を返す新 API に統一する**
  - 新 API は capability を 1 回計算し、戻り値で JSON サイズを返し、バッファが十分なら buf に書き込む。例: `int sora_get_video_codec_capability(const char* config, void* buf, int size)`
  - グローバル `g_video_codec_capability` 自体を廃止する。各呼び出しがローカルに計算して返すため、グローバル state が無くなり、スレッド・Sora インスタンス間の race と 2 段階ペアの横取りが構造的に起きない
  - mutex 案は採用しない。2 段階呼び出しは呼び出しごとの lock ではペアの横取りを防げず、「並行して呼ばれても正しい結果が返る」という完了条件を満たせないため
- C# 側 `Sora.GetVideoCodecCapability` と P/Invoke 宣言を新 API に合わせて置き換える
  - 十分な初期バッファ (例: 64 KB) で 1 回呼び、戻り値がバッファサイズを超えていたら再確保して呼び直す
  - 呼び直しでは capability の再計算が発生するが、グローバル state に依存しないため並行でも正しい結果が返る
- 再利用される `sora_video_codec_preference_*` 系 (has_implementation / merge など) は既に引数渡しでグローバル未使用のため変更しない

## 完了条件

- 2 段階 API と `g_video_codec_capability` が廃止され、1 コールでサイズとデータを返す新 API に置き換わっている
- 複数スレッド、複数 Sora インスタンスから並行して呼ばれても data race が無く、各呼び出しが自分の config に対する正しい結果を返す
- C# 側 `Sora.GetVideoCodecCapability` が新 API を使い、正しく JSON を復元できる
- `CHANGES.md` の `## develop` に `[FIX]` を追記する
