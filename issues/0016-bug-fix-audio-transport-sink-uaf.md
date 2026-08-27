# AudioTransportImpl の audio_sink_ 生ポインタ保持による UAF を修正する

- Priority: Critical
- Created: 2026-08-27
- Branch: fix/audio-transport-sink-uaf
- Polished: {YYYY-MM-DD}
- Milestone: 2026.2.0

## 目的

`AudioTransportImpl::audio_sink_` が `webrtc::AudioTrackSinkInterface*` の生ポインタで sink を保持しており、WebRTC の worker thread が `RecordedDataIsAvailable` で叩いている最中に C# 側の `AudioTrackSinkAdapter.Dispose` が走ると use-after-free になる。併せて `Sora.cs` の `Dispose()` が `sora_destroy → adapter.Dispose` の順序になっている問題も解消する。マイク音量制御やトラック差し替えの操作で踏みうる致命的な UAF。

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

しかしネイティブ側の `AudioTransportImpl::audio_sink_` は `Sora::CreateADM` で作られた時点の `sender_audio_track_sink_` を生ポインタとしてコピー保持しているだけで、C# 側の destroy との同期はない。WebRTC の worker thread は `AudioTransportImpl::RecordedDataIsAvailable` を回し続けており、`audio_sink_` が指す `AudioTrackSinkImpl` が delete されると次の `OnData` 呼び出しで UAF になる。

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

- `AudioTransportImpl::audio_sink_` を生ポインタから `webrtc::scoped_refptr` / `std::shared_ptr` に変更する
  - `AudioTrackSinkInterface` は refcounted であることが多いが、内部実装 (`AudioTrackSinkImpl`) の所有権モデルとの整合を取る必要がある
  - シンプルには `std::shared_ptr<AudioTrackSinkImpl>` を C++ 側で保持し、C# 側の Dispose は shared_ptr の refcount 減少に置き換える
- C# 側 `Sora.cs` `Dispose()` の順序を反転させ、`adapter.Dispose()` を先に呼んでから `sora_destroy(p)` を呼ぶ
  - すべての sink を先に解放してから Sora 本体を destroy する
- `sora_audio_track_sink_destroy` は「sink の登録解除 → adapter が保持する GCHandle と thunk delegate の解放」の順序を保証するように整理する
- 別 issue で扱う `UnityAudioDevice::ProcessAudioData` の thread race と方針を揃える

## 完了条件

- `AudioTransportImpl` の sink 保持が生ポインタから refcount / shared_ptr ベースに変更されている
- C# 側 `Sora.cs` `Dispose()` の順序が「アダプタ Dispose → sora_destroy」に変更されている
- WebRTC worker thread が `OnData` を実行中に C# 側で sink を差し替えても UAF が発生しないことを確認する
- `SenderAudioTrackSink` setter で null を代入するテストシナリオで SEGV しないことを確認する
- `CHANGES.md` の `## develop` に `[FIX] AudioTransportImpl の audio_sink_ UAF を修正する` を追記する
