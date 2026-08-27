# Sora::SetADMVolume の volume を範囲に clamp する

- Priority: Medium
- Created: 2026-08-27
- Branch: fix/set-adm-volume-clamp
- Polished: {YYYY-MM-DD}

## 目的

`src/sora.cpp` の `Sora::SetADMVolume` は入力 volume が `[0.0, 1.0]` の範囲であることを前提にしているが、範囲外値のガードが無い。範囲外値が渡ると uint32_t 型変換で符号なしアンダーフローが起きるため、意図せずスピーカーやマイクの音量が最大化される。C++ 側でも clamp する。

## 現状

`Sora::SetADMVolume` は次の計算を行っている。

- `min_volume + (volume * (max_volume - min_volume))` を uint32_t にキャスト

問題点:

- `volume < 0` の場合、乗算結果が負となり uint32_t キャストで巨大値になる
- `volume > 1.0` の場合も min + (max - min) の範囲を超え、想定外の値が入る
- C# 側の docstring は `[0.0, 1.0]` を要求しているが、C++ 側で契約が守られていることをチェックしていない
- 利用者のミスや Config 設定漏れで、意図せず音量最大化される事故が起きうる

## 設計方針

- 関数先頭で `volume = std::clamp(volume, 0.0, 1.0)` を通す
- 併せて C# 側 docstring と実装の期待範囲を再確認する
- 負値や NaN の扱いをコメントで明記する

## 完了条件

- `SetADMVolume` に `std::clamp` によるガードが入っている
- 範囲外値を渡してもスピーカー / マイク音量が最大化しない
- `CHANGES.md` の `## develop` に `[FIX]` を追記する
