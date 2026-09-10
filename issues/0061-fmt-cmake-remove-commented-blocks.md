# CMakeLists.txt のコメントアウトブロックと dead な install(TARGETS) を削除する

- Priority: Low
- Created: 2026-08-27
- Branch: fmt/cmake-remove-commented-blocks
- Polished: 2026-09-10

## 目的

`CMakeLists.txt` に残っている巨大なコメントアウトブロックと、呼び出し経路の無い `install(TARGETS ...)` を削除する。

## 現状

`CMakeLists.txt` の Windows 分岐と macOS 分岐に、それぞれ `#target_link_libraries(...)` コメントアウトブロックが残っている。Windows 分岐は 26 行、macOS 分岐は 19 行で、合わせて 45 行の dead 記述であり、CMake の設定意図をつかみにくくしている。ライブラリ列は 2022 年の「Sora C++ SDK 化」でコメントアウトされたまま温存されている。現行の `CMakeLists.txt` では Windows / macOS / iOS 分岐のリンクが `Sora::sora` のみであり、このライブラリ列を参照する経路は存在しない。

iOS 分岐には `install(TARGETS SoraUnitySdk DESTINATION lib)` が書かれているが、`run.py` の `_build` は `unity_build_dir/libSoraUnitySdk.a` を `install_file` で `SoraUnitySdkExamples` の Plugins 配下へコピーしており（`buildbase.py` の `install_file` は内部で `shutil.copy2` を利用）、`cmake --install` を呼ぶ経路は存在しない。`.github/workflows/build.yml` にも `cmake --install` は無い。したがってこの `install` 呼び出しは完全に dead。

## 設計方針

- Windows 分岐の `#target_link_libraries` コメントアウトブロックを丸ごと削除する
- macOS 分岐の `#target_link_libraries` コメントアウトブロックを丸ごと削除する
- iOS 分岐の `install(TARGETS SoraUnitySdk DESTINATION lib)` を削除する
- 挙動変更は無い（そもそも dead）

## 完了条件

- 上記のコメントアウトブロックと `install` 行が `CMakeLists.txt` から消えている
- 全ターゲットでビルドが通り、`run.py` のビルド・パッケージング経路に回帰が無い
