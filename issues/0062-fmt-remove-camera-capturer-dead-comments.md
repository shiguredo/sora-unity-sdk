# カメラキャプチャラの死んだコメントアウトブロックを削除する

- Priority: Low
- Created: 2026-08-27
- Branch: fmt/remove-camera-capturer-dead-comments
- Polished: {YYYY-MM-DD}

## 目的

`src/unity_camera_capturer_vulkan.cpp` / `src/unity_camera_capturer_d3d11.cpp` / `src/unity_camera_capturer_d3d12.cpp` に残っている代替実装のコメントアウトブロックを削除し、broken windows を掃除する。

## 現状

`src/unity_camera_capturer_vulkan.cpp` には `VkImageMemoryBarrier` を使ったレイアウト遷移のコメントアウトが 3 セット並び、合計 100 行を超える dead コメントとして残っている。

`src/unity_camera_capturer_d3d11.cpp` の `D3D11Impl::Capture` には別実装案らしき `//libyuv::ARGBToI420(...)` などのコメントアウトブロックが残っている。

`src/unity_camera_capturer_d3d12.cpp` の `D3D12Impl::Capture` には `ResourceBarrier` を無効化する意図のコメントアウトブロックが 2 箇所残っている。

## 設計方針

- Vulkan のレイアウト遷移バリアについては、動作正しさとしての実装復活が別 issue で扱われる（VUID-vkCmdCopyImage-dstImageLayout-00133 対応）。本 issue はその実装復活後にコメントアウト残骸を最終的に消す位置づけで扱う
- D3D11 / D3D12 の代替実装コメントアウトは意図的に残す理由が無いので削除する
- 挙動を変えない範囲でのコメント整理に留める

## 完了条件

- 上記 3 ファイルからコメントアウトブロックが消えている
- ビルドと基本的なキャプチャ動作が回帰していない
- Vulkan の layout barrier 対応 issue との依存関係が本 issue に明記されている
