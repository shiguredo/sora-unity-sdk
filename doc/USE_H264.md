# Sora Unity SDK で H.264 を利用する

Sora Unity SDK では、ソフトウェアでの H.264 エンコード/デコードの機能は提供していません。

ただし、ハードウェアで H.264 エンコーダ/デコーダが使える場合は、それを積極的に利用します。

- Windows 版では [NVIDIA VIDEO CODEC SDK](https://developer.nvidia.com/nvidia-video-codec-sdk) がインストールされていれば、これを利用します。
  - もしこれが利用可能な場合 **H.264 エンコードのみ可能** です。H.264 のデコードはできません。
- macOS 版では [VideoToolbox](https://developer.apple.com/documentation/videotoolbox) を利用します。
  - これは H.264 エンコード/デコードが可能です。また、VP8/VP9 でも VideoToolbox を利用します。

## H.264 が利用可能かどうかを調べる

`Sora.IsH264Supported()` 関数を呼び出すことで、H.264 が利用可能かどうかを調べることができます。

```
bool h264Supported = Sora.IsH264Supported();
```
