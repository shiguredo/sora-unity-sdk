# buildbase.py の未使用関数と hololens2 分岐を削除する

- Priority: Low
- Created: 2026-08-27
- Branch: fmt/cleanup-buildbase-dead-code
- Polished: 2026-09-11

## 目的

`buildbase.py` には Sora Unity SDK から参照されない `install_*` 関数群と `hololens2` 分岐が大量に残っている。これを削除して、ビルドスクリプトを Sora Unity SDK 用に最小化する。

## 現状

`buildbase.py` はビルドスクリプトの共有テンプレート (https://github.com/melpon/buildbase) をコピーしたファイルであり、ファイル冒頭のコメントにもその旨と取り込み方法 (`curl -LO https://raw.githubusercontent.com/melpon/buildbase/master/buildbase.py`) が明記されている。このテンプレートは sora-cpp-sdk など複数プロジェクトで共有されており、`install_amf` や `install_vpl` のように sora-cpp-sdk が実際に使う関数も含まれている。Sora Unity SDK では `run.py` が `from buildbase import` で必要な関数だけを参照しており、それ以外はテンプレート由来のまま未使用で残っている。

`grep` で確認できる範囲では、以下は Sora Unity SDK 内 (`run.py` / `canary.py` / `.github/workflows/build.yml`) から一切呼び出されていない。なお `run.py` には同名の `get_build_platform` 関数があるが、これは `buildbase.py` のものとは無関係の別実装である。

- `install_amf` / `install_sdl2` / `install_sdl3` / `install_cli11`
- `install_cuda_windows` / `install_vpl` / `install_blend2d` / `install_blend2d_official` / `_build_blend2d`
- `install_openh264` / `install_yaml` / `install_catch2`
- `install_grpc` / `install_ggrpc` / `install_spdlog`
- `install_boringssl` / `install_opus` / `install_nasm` / `install_ninja`
- `install_vswhere` / `install_mbedtls` / `install_libjpeg_turbo`
- `install_libyuv` / `install_aom` / `install_rootfs`
- `install_android_sdk_cmdline_tools` / `install_android_sdk_platform_tools`
- `build_and_install_boost`
- `replace_vcproj_static_runtime` / `copytree` / `clone_and_checkout` / `git_get_url_and_revision` / `apply_patch` / `apply_patch_text`
- `PlatformTarget` / `Platform` クラス、`get_windows_osver` / `get_macos_osver` / `get_build_platform` / `get_webrtc_platform` / `add_sora_arguments` / `add_webrtc_build_arguments`
- パッチ定数 `BOOST_PATCH_SUPPORT_14_4` / `GRPC_PATCH_NO_EXECUTABLE` / `BORINGSSL_PATCH_NO_BSSL`
- 未使用 import となる `winreg` (`platform.system() == 'Windows'` の分岐)

`Platform` / `PlatformTarget` クラスは `run.py` から一切参照されておらず、`run.py` は自前の `get_build_platform` / `AVAILABLE_TARGETS` / `BUILD_PLATFORM` でターゲットを判定している。よって `Platform` クラスの Windows 許可リスト (`x86_64` / `arm64` / `hololens2`) にある `hololens2` 分岐も dead である。README には HoloLens 2 のサポート終了が明記されており、ビルドターゲット (`build.yml` の matrix: `windows_x86_64` / `macos_arm64` / `ios` / `ubuntu-22.04_x86_64` / `ubuntu-24.04_x86_64` / `android`) にも `hololens2` は存在しない。

## 設計方針

- 立ち位置は「Sora Unity SDK 用に完全に最小化する」とし、テンプレートをそのまま保持する共有フローにはしない
  - CI で最新テンプレートに上書きするフローは、テンプレート側の変更でビルド内容が勝手に変わって再現性が悪く、ローカルビルドと CI の乖離も生むため採用しない
  - テンプレート側の更新を取り込みたい場合は、手動で差分を適用する
- `run.py` から直接 import されている関数と、それらが間接的に呼ぶ関数・定数だけを残し、残りをすべて削除する
- `run.py` / `canary.py` は変更しない
- `hololens2` 分岐は先に dead となっている `Platform` / `PlatformTarget` クラスごと削除する

## 完了条件

- `buildbase.py` から未使用関数群と `hololens2` 分岐が削除され、`run.py` が直接・間接に使う関数だけが残っている
- `build.yml` の matrix に列挙された全ターゲットでビルドが通り、`run.py` からの呼び出しに回帰が無い
- `CHANGES.md` の `## develop` の `### misc` に、未使用関数の削除を `[UPDATE]` で追記する
