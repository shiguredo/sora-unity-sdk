# D3D12 fence 待機を有限タイムアウト付きに変更する

- Priority: High
- Created: 2026-08-27
- Branch: feature/fix-d3d12-fence-wait-timeout
- Polished: 2026-08-31
- Milestone: 2026.2.0

## 目的

`UnityCameraCapturer::D3D12Impl::Capture` が `WaitForSingleObject(fence_event_, INFINITE)` を使っているため、GPU device removed や GPU ハングで fence が Signal されないと Unity のレンダースレッドが永久にブロックし、ユーザーは Unity プロセスを強制終了するしかなくなる。加えて、待機を有限タイムアウト化してタイムアウトでフレームを破棄する経路を導入すると、次の Capture 冒頭で「GPU 実行が完了していないコマンドリストが参照するアロケータ」に対して `cmd_allocator_->Reset()` を呼ぶことになり、D3D12 ランタイムのエラーでプロセスが異常終了する恐れがある。待機と `cmd_allocator_->Reset()` の順序を正しく設計し直す必要がある。

## 現状

`src/unity_camera_capturer_d3d12.cpp` の `D3D12Impl::Capture` は、ExecuteCommandLists と `queue_->Signal` の後に次のように fence を待機する:

```cpp
if (fence_->GetCompletedValue() < fence_value_) {
  fence_->SetEventOnCompletion(fence_value_, fence_event_);
  WaitForSingleObject(fence_event_, INFINITE);
}
```

`fence_->GetCompletedValue()` による完了確認と、完了済みなら待機をスキップする最適化は既に実装されているが、待機は `INFINITE` のままである。fence が Signal されるまで `WaitForSingleObject` が戻らない。

次の Capture の冒頭では `cmd_allocator_->Reset()` を呼ぶ。現状は前フレームの fence 待機が `INFINITE` のため、前フレームの execute が完了するまで次の Capture の `cmd_allocator_->Reset()` に到達せず、「GPU 実行が完了していないアロケータの Reset」は発生しない。

問題点:

- `INFINITE` 待機のため、GPU がハングしている / device が removed になった場合に Unity レンダースレッドが永久ブロックする
  - ユーザーは Unity プロセスを強制終了するしかなくなる
- 待機を有限タイムアウト化してタイムアウトでフレームを破棄する経路を導入した場合、次の Capture 冒頭で GPU 実行が完了していないアロケータを Reset する経路が生まれ、D3D12 ランタイムのエラーでプロセスが異常終了する恐れがある
  - 現行の `INFINITE` 待機では発生しない回帰リスクであり、タイムアウト化と同時に防ぐ必要がある

## 設計方針

- Capture 冒頭で、前フレームで Signal した fence 値の完了を有限タイムアウト (5 秒) 付きで待つ
  - 完了確認には既存の `fence_->GetCompletedValue()` チェックを維持し、完了済みなら `WaitForSingleObject` をスキップする
  - 初回呼び出し時 (fence_value_ = 0) は待機不要
  - タイムアウト時は警告ログを出し、`cmd_allocator_->Reset()` 以降を実行せず `nullptr` を返す
- fence 待機に成功した場合のみ `cmd_allocator_->Reset()` と `cmd_list_->Reset()` を実行する
- コマンド記録、`cmd_list_->Close()`、`ExecuteCommandLists` の後、`fence_value_++` して `queue_->Signal(fence_, fence_value_)` を実行する
- `readback_buffer_` を Map する前に、現フレームの fence 値の完了を有限タイムアウト (5 秒) 付きで待つ
  - タイムアウト時は警告ログを出し、Map 以降を実行せず `nullptr` を返す
- タイムアウトが繰り返される場合は警告ログを出し続ける (device removed の疑いをユーザーに知らせる)
- device removed / device lost の検出とキャプチャ全体の停止は本 issue の対象外とし、別 issue で対応する

## 完了条件

- `WaitForSingleObject` が有限タイムアウト (5 秒) で返る
- fence が Signal されない状況でも Unity レンダースレッドがハングしない
- fence 待機に失敗したフレームでは `cmd_allocator_->Reset()` 以降が実行されず、GPU 実行が完了していないアロケータが Reset されない
- 待機失敗時は `nullptr` を返し、`readback_buffer_` の Map / 読み出しを行わない
- device removed 相当の GPU ハングを意図的に再現したシナリオで、レンダースレッドが有限時間で復帰し、以後の Capture がクラッシュしないことを確認する
- `CHANGES.md` の `## develop` に `[FIX] D3D12 fence 待機を有限タイムアウト付きに変更する` を追記する
