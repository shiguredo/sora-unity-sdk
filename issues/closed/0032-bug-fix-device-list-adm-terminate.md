# device_list.cpp の ADM を Terminate してから破棄する

- Priority: High
- Created: 2026-08-27
- Completed: 2026-09-01
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

## 解決方法

コード変更は行わず closed にした。issue の前提 (Terminate を呼ばずに破棄するとリークする) が、DEPS が固定する libwebrtc m150 (branch-heads/7871) の一次ソースと矛盾していたため、修正の必要性が成立しない。

一次ソースでの照合結果:

- Windows の `webrtc::CreateWindowsCoreAudioAudioDeviceModule` が返す実クラスは `webrtc_win::WindowsAudioDeviceModule` であり (issue の記述にある `WindowsCoreAudioAudioDeviceModule` というクラスは存在しない)、そのデストラクタは `Terminate()` を明示的に呼ぶ (`modules/audio_device/win/audio_device_module_win.cc` の `~WindowsAudioDeviceModule`)
- issue の記述と逆に、汎用実装 `AudioDeviceModuleImpl::~AudioDeviceModuleImpl` はログ出力のみで `Terminate()` を呼ばない (`modules/audio_device/audio_device_impl.cc`)
- 列挙に使う `IMMDeviceEnumerator` は `CreateDeviceEnumeratorInternal` が呼び出しごとにローカルの `ComPtr` (RAII) として生成し、スコープ終了で解放される (`modules/audio_device/win/core_audio_utility_win.cc`)。列挙の繰り返しで溜まる構造ではない
- macOS の `CreateAudioDeviceModule` (kPlatformDefaultAudio) 経路も `AudioDeviceMac` のデストラクタで後始末が行われる

以上より、`device_list.cpp` の ADM は破棄時にリソースが解放され、ハンドルリークは発生しない。明示的な `adm->Terminate()` の追加は冪等で無害だが、完了条件の「ハンドル数が線形に増えない」「リソース使用量が安定」は修正前に既に満たされており、バグ修正 issue としての根拠が成立しないため対応不要とした。

備考: polish-issue のレビューで本件が処理不能指摘 (issue の前提が一次資料と矛盾) として確定したため、`Polished:` は更新していない。
