# device_list.cpp の ADM を Terminate してから破棄する

- Priority: High
- Created: 2026-08-27
- Branch: fix/device-list-adm-terminate
- Polished: {YYYY-MM-DD}
- Milestone: 2026.2.0

## 目的

`src/device_list.cpp` のデバイス列挙で作成する AudioDeviceModule を `Terminate()` を呼ばずに破棄しているため、WASAPI / CoreAudio 側の COM オブジェクトや IMMDeviceEnumerator がリークする。デバイス列挙を繰り返すたびにハンドルリークが積み上がる。

## 現状

`src/device_list.cpp` は `webrtc::CreateWindowsCoreAudioAudioDeviceModule` および `webrtc::CreateAudioDeviceModule` で AudioDeviceModule を作成し、`adm->Init()` を呼んで列挙処理を行っている。しかし関数を抜ける前に対応する `adm->Terminate()` を呼んでいない。

- `webrtc::AudioDeviceModuleImpl::~AudioDeviceModuleImpl` は Terminate を呼ぶが、`WindowsCoreAudioAudioDeviceModule` のような派生型は独自破棄経路を持つ
- Windows Core Audio では IMMDeviceEnumerator や関連 COM オブジェクトが Terminate 呼び出しを前提に解放される
- デバイス列挙は SDK 起動時に複数回、以降もユーザーの列挙 API 呼び出しごとに走る
- 長時間運用や頻繁な列挙でハンドル数とメモリ使用量が線形に増える

## 設計方針

- `device_list.cpp` の各関数で `adm->Init()` の対称として関数末尾に `adm->Terminate()` を必ず呼ぶ
- 例外パスや途中 return でも Terminate が呼ばれるように RAII ヘルパーを用意する
- 具体的には `RTC_DEFER` 相当の scope guard か、`std::unique_ptr` にカスタム deleter を持たせる

## 完了条件

- `src/device_list.cpp` のすべての関数で ADM の Init と Terminate が対称に呼ばれている
- デバイス列挙を N 回繰り返しても COM オブジェクトのハンドル数が線形に増えない
- Windows と macOS で列挙 100 回連続実行後にリソース使用量が安定していることを確認する
- `CHANGES.md` の `## develop` に `[FIX]` を追記する
