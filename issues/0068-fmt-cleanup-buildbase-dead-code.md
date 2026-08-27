# buildbase.py の dead 関数と hololens2 分岐を掃除する

- Priority: Low
- Created: 2026-08-27
- Branch: fmt/cleanup-buildbase-dead-code
- Polished: {YYYY-MM-DD}

## 目的

`buildbase.py` に大量に残っている未使用の `install_*` 関数と `hololens2` 分岐を整理する。テンプレート由来のファイルとして残す方針か、Sora Unity SDK 用に最小化する方針かを含めて立ち位置を明確にする。

## 現状

`buildbase.py` には `install_amf` を含む多数の `install_*` 関数が定義されているが、Sora Unity SDK 内から呼ばれていない。`grep` で確認できる範囲では以下が未使用となっている。

- `install_amf` / `install_sdl2` / `install_sdl3` / `install_cli11`
- `install_cuda_windows` / `install_vpl` / `install_blend2d`
- `install_openh264` / `install_yaml` / `install_catch2`
- `install_grpc` / `install_ggrpc` / `install_spdlog`
- `install_boringssl` / `install_opus` / `install_nasm` / `install_ninja`
- `install_vswhere` / `install_mbedtls` / `install_libjpeg_turbo`
- `install_libyuv` / `install_aom` / `install_rootfs`
- `build_and_install_boost`
- `PlatformTarget` / `Platform` クラス

さらに Windows target の許可リストに `hololens2` を並べた分岐が `buildbase.py` に残っている。README では HoloLens 2 サポート終了を明記しているのに、受け入れ側だけが生きている状態。

## 設計方針

- `buildbase.py` の立ち位置を明確化する
  - 立ち位置 A: sora-cpp-sdk 由来のテンプレートとして扱い、CI で `curl` などで最新に上書きするフローに切り替える
  - 立ち位置 B: Sora Unity SDK 用に完全に最小化し、必要な関数だけ残す
- どちらの立ち位置でも `hololens2` 分岐は Sora Unity SDK 側では削除して構わない
- 立ち位置 B を採用する場合、`run.py` から実際に呼ばれる関数だけを残し、未使用関数を全削除する
- CHANGES.md には方針変更を記録する

## 完了条件

- `buildbase.py` の立ち位置がどちらかに決まっている
- 未使用の `install_*` 関数群と `hololens2` 分岐が整理されている
- `run.py` からの呼び出しに回帰が無く、全ターゲットでビルドが通る
- `CHANGES.md` の `## develop` に該当記述を追記する
