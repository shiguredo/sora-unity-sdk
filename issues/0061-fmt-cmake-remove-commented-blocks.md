# CMakeLists.txt のコメントアウトブロックと dead な install(TARGETS) を削除する

- Priority: Low
- Created: 2026-08-27
- Branch: fmt/cmake-remove-commented-blocks
- Polished: {YYYY-MM-DD}

## 目的

`CMakeLists.txt` に残っている巨大なコメントアウトブロックと、呼び出し経路の無い `install(TARGETS ...)` を削除する。

## 現状

`CMakeLists.txt` の Windows 分岐と macOS 分岐に、それぞれ 20 行超の `#target_link_libraries(...)` コメントアウトブロックが残っている。両分岐で合計 40 行以上の dead 記述であり、CMake の設定意図をつかみにくくしている。Sora C++ SDK 側で解決されているはずのライブラリ列がコメントアウトで温存されている状態。

iOS 分岐には `install(TARGETS SoraUnitySdk DESTINATION lib)` が書かれているが、`run.py` の `_build` は `unity_build_dir/libSoraUnitySdk.a` を `shutil.copy` で直接コピーしており、`cmake --install` を呼ぶ経路は存在しない。`.github/workflows/build.yml` にも `cmake --install` は無い。したがってこの `install` 呼び出しは完全に dead。

## 設計方針

- Windows 分岐の `#target_link_libraries` コメントアウトブロックを丸ごと削除する
- macOS 分岐の `#target_link_libraries` コメントアウトブロックを丸ごと削除する
- iOS 分岐の `install(TARGETS SoraUnitySdk DESTINATION lib)` を削除する
- 挙動変更は無い（そもそも dead）

## 完了条件

- 上記のコメントアウトブロックと `install` 行が `CMakeLists.txt` から消えている
- 全ターゲットでビルドが通り、`run.py` のビルド・パッケージング経路に回帰が無い
