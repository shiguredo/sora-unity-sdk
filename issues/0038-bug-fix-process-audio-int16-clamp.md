# UnityAudioDevice::ProcessAudioData の float から int16 変換で clamp する

- Priority: High
- Created: 2026-08-27
- Branch: feature/fix-process-audio-int16-clamp
- Polished: 2026-09-10
- Milestone: 2026.2.0

## 目的

`src/unity_audio_device.h` の `ProcessAudioData` が入力を `[-1, 1]` 前提でスケールしているため、Unity Mixer による増幅や Reverb 経路で範囲外サンプルが入ると `int16` 変換で未定義動作になる。`std::clamp` を通して安全側に丸める。

## 現状

`UnityAudioDevice::ProcessAudioData` は Unity から受け取った float サンプルを次のようにスケールしている。

- `data[i] >= 0 ? data[i] * SHRT_MAX : data[i] * -SHRT_MIN`

問題点:

- 入力は `SoraSample.cs` の `AudioRenderer.Render` で取得した Unity の出力音声であり、AudioMixer の増幅や Reverb、Distortion 等のエフェクトを経由すると `[-1, 1]` を超える値を含み得る
- `[INT16_MIN, INT16_MAX]` を超える float から int16 への暗黙変換は C++ 規格上未定義動作になる
- 結果として Opus 経路にノイズやクリッピングとして伝播する
- 現在の `push_back(...)` は `#pragma warning(suppress : 4244)` で MSVC の変換警告を抑制しているだけで、実挙動は保証されていない

## 設計方針

- スケール後の値を `[SHRT_MIN, SHRT_MAX]` に `std::clamp` で丸め、明示的な `static_cast<int16_t>` で変換する
- あるいは入力を `[-1.0f, 1.0f]` に `std::clamp` してからスケールし、同様に明示的な `static_cast<int16_t>` で変換する
- どちらの場合も `#pragma warning(suppress : 4244)` を削除し、正しい変換にする

## 完了条件

- `ProcessAudioData` が範囲外 float 入力を安全に扱う
- int16 範囲を超える値は clamp されて Opus へ渡る
- Unity Mixer で増幅した音声を送信してクリッピングノイズが減る
- `CHANGES.md` の `## develop` に `[FIX]` を追記する
