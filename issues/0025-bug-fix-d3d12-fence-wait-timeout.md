# D3D12 fence 待機を有限タイムアウト付きに変更する

- Priority: High
- Created: 2026-08-27
- Branch: fix/d3d12-fence-wait-timeout
- Polished: {YYYY-MM-DD}
- Milestone: 2026.2.0

## 目的

`UnityCameraCapturer::D3D12Impl::Capture` が `WaitForSingleObject(fence_event_, INFINITE)` を使っているため、GPU device removed などで fence が Signal されないと Unity のレンダースレッドが永久にブロックする。加えて、前フレームの `cmd_list_` が execute 未完了の状態で `cmd_allocator_->Reset()` を呼ぶと D3D12 ランタイム側のエラーでプロセスが abort する。

## 現状

`src/unity_camera_capturer_d3d12.cpp` の `D3D12Impl::Capture` は次のように fence を待機する:

```cpp
WaitForSingleObject(fence_event_, INFINITE);
```

続けて次回 Capture の準備として `cmd_allocator_->Reset()` を呼ぶ:

```cpp
cmd_allocator_->Reset();
```

問題点:

- `INFINITE` 待機のため、GPU が hang している / device が removed になった場合に Unity レンダースレッドが永久ブロックする
  - ユーザーは Unity プロセスを強制終了するしかなくなる
- 前フレームの `cmd_list_` が GPU 側で execute 中に `cmd_allocator_->Reset()` を呼ぶと、D3D12 デバッグレイヤが `ERROR: The command allocator cannot be reset while there are outstanding command lists` を吐いてプロセスを落とす

## 設計方針

- `WaitForSingleObject` に有限タイムアウトを設定する (例: 5 秒)
  - タイムアウト時は Capture を中断してエラーを返す (`nullptr`)
  - タイムアウトが繰り返された場合の警告ログを出す
- `WaitForSingleObject` の前に `fence_->GetCompletedValue()` で fence の完了状態を確認し、既に完了していれば `WaitForSingleObject` をスキップする
- `cmd_allocator_->Reset()` は fence 待機成功後にのみ呼ぶ (待機失敗時は skip する)
- device removed や device lost 状態を検出して `capturer` 全体を停止するフォールバックを検討する

## 完了条件

- `WaitForSingleObject` が有限タイムアウトで返る
- fence が Signal されない状況で Unity レンダースレッドがハングしない
- 前フレームの `cmd_list_` execute 未完了時に `cmd_allocator_->Reset()` を呼ばない
- Windows で意図的に GPU 高負荷をかけたシナリオでキャプチャが safely fail することを確認する
- `CHANGES.md` の `## develop` に `[FIX] D3D12 fence 待機を有限タイムアウト付きに変更する` を追記する
