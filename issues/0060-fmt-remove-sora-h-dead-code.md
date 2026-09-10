# src/sora.h と src/sora.cpp の死抽象と未使用 include を削除する

- Priority: Low
- Created: 2026-08-27
- Branch: fmt/remove-sora-h-dead-code
- Polished: 2026-09-10

## 目的

`src/sora.h` に残っている死抽象、未初期化 / 未使用のメンバ、未使用 include をまとめて削除し、`src/converter.cpp` の未使用 include も併せて掃除する。

## 現状

`src/sora.h` には次の死コードが残っている。

- `struct CapturerSink : webrtc::VideoSinkInterface<webrtc::VideoFrame>` の宣言全体
  - コンストラクタ / デストラクタ / `OnFrame` の実装が `src/sora.cpp` に存在しない
  - 実際にインスタンス化する経路もない
- `std::shared_ptr<CapturerSink> capturer_sink_` メンバ
- `~Sora` 内の `capturer_sink_ = nullptr;`
- `webrtc::TaskQueueFactory* task_queue_factory_` メンバ
  - コンストラクタ初期化子にも `= nullptr` 初期化にも入っておらず、参照する箇所も無い
  - git 履歴で確認した限り、2022 年の「Sora C++ SDK 化」で追加されて以降、`src/sora.cpp` / `src/unity.cpp` のどこからも一度も参照されていない
- 未使用 include: `#include <thread>` / `#include <media/engine/webrtc_media_engine.h>`
- 削除とともに不要になる include: `#include <api/task_queue/task_queue_factory.h>`（`task_queue_factory_` の型定義にのみ使用されている）

`src/converter.cpp` の `#include <sora/vpl_session.h>` も `VplSession` の未使用で残骸となっている。

## 設計方針

- 上記の宣言・メンバ・行・include を全て削除する
- 削除後にビルドが通ることを全ターゲットで確認する
- 挙動変更は無い（そもそも死コード）

## 完了条件

- `src/sora.h` から `struct CapturerSink` / `capturer_sink_` / `task_queue_factory_` が消えている
- `src/sora.h` から削除対象の include 3 件が消えている
- `src/sora.cpp` の `~Sora` から `capturer_sink_ = nullptr;` が消えている
- `src/converter.cpp` から `#include <sora/vpl_session.h>` が消えている
- Windows / macOS / iOS / Android / Ubuntu の全ターゲットでビルドが通る
