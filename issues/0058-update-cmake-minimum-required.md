# CMakeLists.txt の cmake_minimum_required を実態に合わせて引き上げる

- Priority: Medium
- Created: 2026-08-27
- Branch: update/cmake-minimum-required
- Polished: 2026-09-10

## 目的

`CMakeLists.txt` の `cmake_minimum_required` は 2020 年から `VERSION 3.16` のままで、実際のビルドで使われる CMake と依存する sora-cpp-sdk の最低要件を下回っている。宣言される最低要件を実態に沿うバージョンへ引き上げる。

## 現状

`CMakeLists.txt` の先頭は `cmake_minimum_required(VERSION 3.16)` であり、`cca5499` (2020-02-11) から変更されていない。

実際のビルドでは、`run.py` が DEPS の `CMAKE_VERSION=4.4.2` を取得し、PATH に追加した CMake を使用する。また、依存する sora-cpp-sdk (DEPS の `SORA_CPP_SDK_VERSION=2026.2.1`) は自身の `CMakeLists.txt` で `cmake_minimum_required(VERSION 3.23)` を宣言しており、`CMP0054` / `CMP0091` も sora-unity-sdk と同じポリシー構成である。

`SoraUnitySdk` ターゲットは `CXX_STANDARD 20` を要求しているが、CXX_STANDARD 20 は CMake 3.12 で追加されており、CMake 3.16 でも問題なく使える。機能面で 3.16 が不足しているわけではないが、3.16 は依存する sora-cpp-sdk の最低要件 (3.23) より古く、実態を正しく表していない。

ポリシーについても問題は無い。CMake 4.0 は `cmake_minimum_required()` への 3.5 未満の指定をエラーにするが、3.16 は対象外である。`CMP0054` (3.1 導入) と `CMP0091` (3.15 導入) は `cmake_minimum_required(VERSION 3.16)` 以上の宣言で既定 NEW となり、明示指定済みでもある。

## 設計方針

- `cmake_minimum_required` を `VERSION 3.23` に更新する
  - 依存する sora-cpp-sdk 2026.2.1 の最低要件と一致させるため
  - sora-cpp-sdk は 3.23 を宣言した状態で運用されており、3.23 は 2022-03-29 リリースのバージョンである
  - DEPS の `CMAKE_VERSION=4.4.2` は `run.py` が使うビルド時の CMake バージョンであり最低要件ではないため、`cmake_minimum_required` には 4.4.2 を設定しない
  - ローカルでシステムの CMake を使ってビルドする場合は 3.23 以上が必要になるが、公式のビルド経路 (run.py) は DEPS の CMake を使うため影響しない
- ポリシー設定は現状維持とする (`CMP0054` / `CMP0091` の明示を残し、追加・削除しない)
  - `cmake_minimum_required(VERSION 3.23)` でも両ポリシーの挙動は変わらない
- 変更は `CMakeLists.txt` の `cmake_minimum_required` の値のみで、ビルド挙動は変わらない

## 完了条件

- `CMakeLists.txt` の `cmake_minimum_required` が `VERSION 3.23` に更新されている
- `.github/workflows/build.yml` の全ターゲット (windows_x86_64 / macos_arm64 / ios / android / ubuntu-22.04_x86_64 / ubuntu-24.04_x86_64) でビルドが成功する
- `CHANGES.md` の `## develop` に当該変更が記載されている
