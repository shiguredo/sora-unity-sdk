# CMakeLists.txt の cmake_minimum_required を実態に合わせて引き上げる

- Priority: Medium
- Created: 2026-08-27
- Branch: update/cmake-minimum-required
- Polished: {YYYY-MM-DD}

## 目的

`CMakeLists.txt` の `cmake_minimum_required` を実態のビルド要件に沿うバージョンまで引き上げ、DEPS の `CMAKE_VERSION` との整合をとる。

## 現状

`CMakeLists.txt` の先頭には `cmake_minimum_required(VERSION 3.16)` と書かれている。

一方、実際のビルドで使用する CMake は DEPS の `CMAKE_VERSION=4.4.2` で指定されており、`SoraUnitySdk` は `CXX_STANDARD 20` を要求している。C++20 サポートを含むターゲット指定と CMake 3.16 の乖離が大きい。

CMake 4.x では複数のポリシーのデフォルトが変わっており、旧バージョンの互換性を明示的に許容していると将来のリベースで意図しない挙動になる可能性がある。`CMP0054` / `CMP0091` は明示指定されているが、他ポリシーも整理する余地がある。

## 設計方針

- `cmake_minimum_required` を 3.20 以上、実運用実績のあるバージョンに引き上げる（sora-cpp-sdk との整合も確認する）
- CMake 4.x のポリシー既定変更に対する対応が必要なら合わせて明示する
- CI（`.github/workflows/build.yml`）が要求 CMake バージョンを満たしていることを確認する

## 完了条件

- `cmake_minimum_required` が `CXX_STANDARD 20` の要求と整合するバージョンに更新されている
- CI 上の全プラットフォームでビルドが通る
- `CHANGES.md` の `## develop` に該当記述を追記する
