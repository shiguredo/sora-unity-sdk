# Sora Unity SDK で H.264 を利用する

Sora Unity SDK では、ソフトウェアでの H.264 エンコード/デコードの機能は提供していません。

ただし、ハードウェアで H.264 エンコーダ/デコーダが使える場合は、それを積極的に利用します。

- Windows 版では [NVIDIA VIDEO CODEC SDK](https://developer.nvidia.com/nvidia-video-codec-sdk) または [Intel Media SDK](https://www.intel.com/content/www/us/en/developer/tools/media-sdk/overview.html) がインストールされていれば、これを利用します。
- macOS, iOS 版では [VideoToolbox](https://developer.apple.com/documentation/videotoolbox) を利用します。
- Android 版では [MediaCodec](https://developer.android.com/reference/android/media/MediaCodec) で H.264 が利用可能であれば利用します。
- Linux 版では NVIDIA VIDEO CODEC SDK がインストールされていれば、これを利用します。

## H.264 が利用可能かどうかを調べる

`Sora.IsH264Supported()` 関数を呼び出すことで、H.264 が利用可能かどうかを調べることができます。

```
bool h264Supported = Sora.IsH264Supported();
```
