# iOS Connect 経路で on_disconnect が二重ムーブされ空 function 化する問題を修正する

- Priority: Critical
- Created: 2026-08-27
- Branch: fix/ios-on-disconnect-double-move
- Polished: {YYYY-MM-DD}
- Milestone: 2026.2.0

## 目的

iOS で `Sora::Connect` を呼び出した際、エラー分岐で `on_disconnect` を呼び出すと `std::bad_function_call` でクラッシュする致命バグを修正する。iOS の主要な接続経路 (`sendonly` / `sendrecv` かつ `unity_audio_input = false`) で発火する経路であり、正式リリース前に必ず潰す必要がある。

## 現状

`src/sora.cpp` の `Sora::Connect` の iOS 分岐で、`IosAudioInit` のコールバック lambda に `on_disconnect` を init capture でムーブしている:

```cpp
IosAudioInit(
    [this, on_disconnect = std::move(on_disconnect)](std::string error) {
      if (!error.empty()) {
        on_disconnect((int)sora_conf::ErrorCode::INTERNAL_ERROR,
                      "Failed to IosAudioInit: error=" + error);
      }
      ios_audio_initializing = false;
    });
```

その直後、同じ `on_disconnect` を再度ムーブして `DoConnect` に渡している:

```cpp
DoConnect(cc, std::move(on_disconnect));
```

C++ の init capture (`on_disconnect = std::move(on_disconnect)`) は外側の `on_disconnect` を moved-from 状態にする。`std::function` の moved-from は空関数として扱われる仕様のため、`DoConnect` には空関数が渡ることになる。

`DoConnect` 内でエラー分岐 (`role` 未対応 / `signaling_url` 空 等) に入ると `on_disconnect((int)sora_conf::ErrorCode::INVALID_PARAMETER, ...)` を呼び出すため、空関数呼び出しで `std::bad_function_call` 例外が送出されプロセスがクラッシュする。

さらに、この分岐で使用している `static bool ios_audio_initializing` はプロセス全域の共有状態で `std::atomic` でもなく、複数の `Sora` インスタンスが並列に `Connect` を呼び出すと read-modify-write の race も発生する。`IosAudioInit` のコールバック内で `[this, ...]` を捕捉している点も、Sora の破棄後にコールバックが到着した場合に use-after-free になる。

## 設計方針

- `on_disconnect` を lambda にコピーキャプチャに変更し、外側の変数を moved-from にしない
  - あるいは、`IosAudioInit` の完了後に `DoConnect` を呼ぶ設計 (シリアル化) に変更する
- 初期化と接続の順序を明確にする
  - 現状は `IosAudioInit` の完了を待たずに `DoConnect` を呼んでおり、マイク初期化前に signaling が開始してしまう
  - コールバック内で `DoConnect` を呼ぶ形にリファクタリングすることで、初期化と接続の順序契約を保証する
- `static bool ios_audio_initializing` を `std::atomic<bool>` にする
  - 単純な bool のままだと複数 Sora インスタンスの並列 Connect で race する
- `IosAudioInit` のコールバックで捕捉する `this` を `weak_ptr<Sora>` (すなわち `weak_from_this()`) 経由に変更する
  - Sora 破棄後にコールバックが到着した場合に UAF を回避する

## 完了条件

- iOS で `Connect` を呼び出し、`role` を意図的に不正値にするなど `on_disconnect` を発火させるエラー経路で `std::bad_function_call` が発生しないことを確認する
- `IosAudioInit` の完了前に `DoConnect` が実行される競合状態が解消されている
- 複数 Sora インスタンスの並列 `Connect` で `ios_audio_initializing` の書き換え競合が発生しないことをコードレベルで保証する (atomic 化 + weak_ptr 化)
- iOS 実機で `sendonly` / `sendrecv` の主要接続経路 (`unity_audio_input = false`) の疎通確認が取れている
- `CHANGES.md` の `## develop` に `[FIX] iOS Connect 経路の on_disconnect 二重ムーブによるクラッシュを修正する` を追記する
