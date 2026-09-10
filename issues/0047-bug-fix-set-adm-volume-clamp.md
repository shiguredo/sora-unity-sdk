# Sora::SetADMVolume の volume を範囲に clamp する

- Priority: Medium
- Created: 2026-08-27
- Branch: feature/fix-set-adm-volume-clamp
- Polished: 2026-09-10

## 目的

`src/sora.cpp` の `Sora::SetADMVolume` は入力 volume が `[0.0, 1.0]` の範囲であることを前提にしているが、範囲外値のガードが無い。負値や 1.0 超の値が渡ると uint32_t 型変換で巨大値になり、意図せずスピーカーやマイクの音量が最大化される。NaN は浮動小数から整数への変換結果自体が保証されないため、同じく安全に扱う必要がある。C++ 側でもガードする。

## 現状

`Sora::SetADMVolume` は次の計算を行っている。

- `min_volume + (volume * (max_volume - min_volume))` を uint32_t にキャスト

問題点:

- `volume < 0` の場合、乗算結果が負となり uint32_t 変換で巨大値になる
- `volume > 1.0` の場合も min + (max - min) の範囲を超え、想定外の値が入る
- NaN は `std::clamp` でも矯正されず（比較がすべて false になり NaN のまま返る）、uint32_t への静的キャストの変換結果が保証されない
- C# 側の docstring は `[0.0, 1.0]` を要求しているが、C++ 側で契約が守られていることをチェックしていない
- 利用者のミスや Config 設定漏れで、意図せず音量最大化される事故が起きうる

## 設計方針

- 関数先頭で `volume = std::clamp(volume, 0.0, 1.0)` を通す
- NaN は `std::clamp` では矯正できないため、`std::isnan` で検出して `false` を返す（不正な入力として拒否する）
- 負値や NaN の扱いをコメントで明記する
- C# 側の docstring は `Sora.cs` の `SetSpeakerVolume` / `SetMicrophoneVolume` が `[0.0, 1.0]` を要求しており、clamp の期待範囲と一致している（変更不要。確認済み）

## 完了条件

- `SetADMVolume` に `std::clamp` によるガードが入っている
- 範囲外値や NaN を渡してもスピーカー / マイク音量が最大化しない
- `CHANGES.md` の `## develop` に `[FIX]` を追記する
