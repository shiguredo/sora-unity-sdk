# AudioTransportImpl の audio_sink_ 生ポインタ保持による UAF を修正する

- Priority: Critical
- Created: 2026-08-27
- Branch: feature/fix-audio-transport-sink-uaf
- Polished: 2026-08-29
- Milestone: 2026.2.0

## 目的

`AudioTransportImpl::audio_sink_` が `webrtc::AudioTrackSinkInterface*` の生ポインタで sink を保持しており、オーディオスレッド (Unity 音声入力時は Unity のオーディオスレッド、デバイス録音時は ADM の録音スレッド) が `RecordedDataIsAvailable` で叩いている最中に C# 側の `AudioTrackSinkAdapter.Dispose` が走ると use-after-free になる。併せて `Sora.cs` の `Dispose()` が `sora_destroy → adapter.Dispose` の順序になっている問題も解消する。トラック差し替えの操作で踏みうる致命的な UAF。

## 現状

`src/unity_audio_device.h` の `AudioTransportImpl` は次のように `audio_sink_` を生ポインタで保持している:

```cpp
struct AudioTransportImpl : public webrtc::AudioTransport {
  AudioTransportImpl(webrtc::AudioTrackSinkInterface* audio_sink)
      : audio_sink_(audio_sink) {}
  ...
  int32_t RecordedDataIsAvailable(...) override {
    if (audio_sink_ != nullptr) {
      audio_sink_->OnData(audioSamples, nBytesPerSample * 8, samplesPerSec,
                          nChannels, nSamples, std::nullopt);
    }
    ...
  }

 private:
  webrtc::AudioTransport* audio_transport_ = nullptr;
  webrtc::AudioTrackSinkInterface* audio_sink_ = nullptr;
};
```

C# 側 `SoraUnitySdkExamples/Assets/SoraUnitySdk/Sora.cs` の `SenderAudioTrackSink` setter は次のような流れで sink を差し替える:

```csharp
sora_set_sender_audio_track_sink(this.p, IntPtr.Zero);
adapter.Dispose();  // 内部で sora_audio_track_sink_destroy → AudioTrackSinkImpl を delete
```

しかしネイティブ側の `AudioTransportImpl::audio_sink_` は `Sora::CreateADM` で作られた時点の `sender_audio_track_sink_` を生ポインタとしてコピー保持しているだけで、C# 側の destroy との同期はない。`RecordedDataIsAvailable` は Unity 音声入力時は Unity のオーディオスレッドから、デバイス録音時は ADM の録音スレッドから呼ばれ続けており、`audio_sink_` が指す `AudioTrackSinkImpl` が delete されると次の `OnData` 呼び出しで UAF になる。

さらに C# 側 `Sora.cs` の `Dispose()` は次の順序で処理する:

```csharp
sora_destroy(p);
p = IntPtr.Zero;
...
foreach (var adapter in audioTrackSinks.Values) adapter.Dispose();
if (senderAudioTrackSinkAdapter != null) senderAudioTrackSinkAdapter.Dispose();
```

`sora_destroy` は SoraWrapper を delete するが、内部の `std::shared_ptr<Sora>` が最後の参照でなければ `~Sora` は動かない。`GetStats` などで shared_from_this() した参照が残っていれば、`sora_destroy` 後も生きている `Sora` の `AudioTransportImpl::audio_sink_` が dangling 参照になる。

## 設計方針

- `AudioTransportImpl::audio_sink_` を生ポインタから `std::shared_ptr<AudioTrackSinkImpl>` に変更する
  - `webrtc::AudioTrackSinkInterface` は refcounted ではないため `webrtc::scoped_refptr` では保持できない。`AudioTrackSinkImpl` を `std::shared_ptr` で保持する
  - C# 側の `AudioTrackSinkAdapter.Dispose` は `AudioTrackSinkImpl` の破棄ではなく C++ 側の参照カウント減少に置き換え、`AudioTransportImpl` が参照を保持している限り `AudioTrackSinkImpl` が破棄されないようにする
- sink の差し替え・null 代入を `AudioTransportImpl::audio_sink_` に反映する経路を設ける
  - `Sora::SetSenderAudioTrackSink` は `sender_audio_track_sink_` の更新だけでなく、`UnityAudioDevice` 経由で `AudioTransportImpl` が保持する参照も差し替える
  - `RecordedDataIsAvailable` が実行されるスレッド (Unity オーディオスレッド / ADM の録音スレッド) と、差し替え側の同期を取る
- C# 側 `Sora.cs` `Dispose()` の順序を「adapter の破棄 → `sora_destroy(p)`」に変更する
  - すべての sink を先に解放してから Sora 本体を destroy する
- `sora_audio_track_sink_destroy` と C# 側 `AudioTrackSinkAdapter.Dispose` の責務分担を明確にする
  - GCHandle と thunk delegate は C# 側 `AudioTrackSinkAdapter` が解放し、ネイティブ側は `AudioTrackSinkImpl` の参照カウント減少のみを行う
- `SenderAudioTrackSink` setter の docstring にある「Connect() 後に値を書き換えた場合の挙動は未定義動作」の記述を、差し替えが安全に行える旨に更新する
- 別 issue で扱う `UnityAudioDevice::ProcessAudioData` の thread race と方針を揃える

## 完了条件

- `AudioTransportImpl` の sink 保持が生ポインタから `std::shared_ptr` ベースに変更されている
- sink の差し替え・null 代入が `AudioTransportImpl` の保持する参照に反映され、旧 sink が参照カウントで保護されている
- C# 側 `Sora.cs` `Dispose()` の順序が「アダプタ破棄 → sora_destroy」に変更されている
- `RecordedDataIsAvailable` の実行中に C# 側で sink を差し替えても UAF が発生しないことを確認する
- `SenderAudioTrackSink` setter で null を代入するテストシナリオで SEGV しないことを確認する
- `SenderAudioTrackSink` setter の docstring が更新され、「Connect() 後の差し替え」が安全に行える旨になっている
- `CHANGES.md` の `## develop` に `[FIX] AudioTransportImpl の audio_sink_ UAF を修正する` を追記する
