﻿using System;
using System.Runtime.InteropServices;

public class Sora : IDisposable
{
    public enum Role
    {
        Sendonly,
        Recvonly,
        Sendrecv,
    }
    public enum CapturerType
    {
        DeviceCamera = 0,
        UnityCamera = 1,
    }
    public enum VideoCodecType
    {
        VP9,
        VP8,
        H264,
        AV1,
    }
    public enum AudioCodecType
    {
        OPUS,
    }
    public class Config
    {
        public string SignalingUrl = "";
        public string ChannelId = "";
        public string Metadata = "";
        public Role Role = Sora.Role.Sendonly;
        public bool Multistream = false;
        public bool Spotlight = false;
        public int SpotlightNumber = 0;
        public bool Simulcast = false;
        public CapturerType CapturerType = Sora.CapturerType.DeviceCamera;
        public UnityEngine.Camera UnityCamera = null;
        public int UnityCameraRenderTargetDepthBuffer = 16;
        public string VideoCapturerDevice = "";
        public int VideoWidth = 640;
        public int VideoHeight = 480;
        public VideoCodecType VideoCodecType = VideoCodecType.VP9;
        public int VideoBitRate = 0;
        public bool UnityAudioInput = false;
        public bool UnityAudioOutput = false;
        public string AudioRecordingDevice = "";
        public string AudioPlayoutDevice = "";
        public AudioCodecType AudioCodecType = AudioCodecType.OPUS;
        public int AudioBitRate = 0;

        // DataChannel を使ったシグナリングを利用するかどうか
        public bool DataChannelSignaling = false;
        // DataChannel のデータが何秒間やって来なければ切断したとみなすか
        public int DataChannelSignalingTimeout = 30;
        // DataChannel の接続が確立した後は、WebSocket が切断されても Sora との接続を確立したままとするかどうか
        public bool IgnoreDisconnectWebsocket = false;
        // DataChannel の接続が確立したら、クライアントから明示的に WebSocket を切断するかどうか
        // （IgnoreDisconnectWebsocket=true の場合のみ有効）
        public bool CloseWebsocket = true;
    }

    IntPtr p;
    GCHandle onAddTrackHandle;
    GCHandle onRemoveTrackHandle;
    GCHandle onNotifyHandle;
    GCHandle onPushHandle;
    GCHandle onHandleAudioHandle;
    UnityEngine.Rendering.CommandBuffer commandBuffer;
    UnityEngine.Camera unityCamera;

    public void Dispose()
    {
        if (onAddTrackHandle.IsAllocated)
        {
            onAddTrackHandle.Free();
        }

        if (onRemoveTrackHandle.IsAllocated)
        {
            onRemoveTrackHandle.Free();
        }

        if (onNotifyHandle.IsAllocated)
        {
            onNotifyHandle.Free();
        }

        if (onPushHandle.IsAllocated)
        {
            onPushHandle.Free();
        }

        if (p != IntPtr.Zero)
        {
            sora_destroy(p);
            p = IntPtr.Zero;
        }

        // Sora が別スレッドからコールバックを呼びまくっているので、
        // Sora を破棄した後に解放しないとエラーになってしまう。
        if (onHandleAudioHandle.IsAllocated)
        {
            onHandleAudioHandle.Free();
        }
    }

    public Sora()
    {
        p = sora_create();
        commandBuffer = new UnityEngine.Rendering.CommandBuffer();
    }

    public bool Connect(Config config)
    {
        IntPtr unityCameraTexture = IntPtr.Zero;
        if (config.CapturerType == CapturerType.UnityCamera)
        {
            unityCamera = config.UnityCamera;
            var texture = new UnityEngine.RenderTexture(config.VideoWidth, config.VideoHeight, config.UnityCameraRenderTargetDepthBuffer, UnityEngine.RenderTextureFormat.BGRA32);
            unityCamera.targetTexture = texture;
            unityCamera.enabled = true;
            unityCameraTexture = texture.GetNativeTexturePtr();
        }

        var role =
            config.Role == Role.Sendonly ? "sendonly" :
            config.Role == Role.Recvonly ? "recvonly" : "sendrecv";
        return sora_connect(
            p,
            UnityEngine.Application.unityVersion,
            config.SignalingUrl,
            config.ChannelId,
            config.Metadata,
            role,
            config.Multistream ? 1 : 0,
            config.Spotlight ? 1 : 0,
            config.SpotlightNumber,
            config.Simulcast ? 1 : 0,
            (int)config.CapturerType,
            unityCameraTexture,
            config.VideoCapturerDevice,
            config.VideoWidth,
            config.VideoHeight,
            config.VideoCodecType.ToString(),
            config.VideoBitRate,
            config.UnityAudioInput ? 1 : 0,
            config.UnityAudioOutput ? 1 : 0,
            config.AudioRecordingDevice,
            config.AudioPlayoutDevice,
            config.AudioCodecType.ToString(),
            config.AudioBitRate,
            config.DataChannelSignaling ? 1 : 0,
            config.DataChannelSignalingTimeout,
            config.IgnoreDisconnectWebsocket ? 1 : 0,
            config.CloseWebsocket ? 1 : 0) == 0;
    }

    // Unity 側でレンダリングが完了した時（yield return new WaitForEndOfFrame() の後）に呼ぶイベント
    // 指定した Unity カメラの映像を Sora 側のテクスチャにレンダリングしたりする
    public void OnRender()
    {
        UnityEngine.GL.IssuePluginEvent(sora_get_render_callback(), sora_get_render_callback_event_id(p));
    }

    // trackId で受信した映像を texutre にレンダリングする
    public void RenderTrackToTexture(uint trackId, UnityEngine.Texture texture)
    {
        commandBuffer.IssuePluginCustomTextureUpdateV2(sora_get_texture_update_callback(), texture, trackId);
        UnityEngine.Graphics.ExecuteCommandBuffer(commandBuffer);
        commandBuffer.Clear();
    }

    private delegate void TrackCallbackDelegate(uint track_id, IntPtr userdata);

    [AOT.MonoPInvokeCallback(typeof(TrackCallbackDelegate))]
    static private void TrackCallback(uint trackId, IntPtr userdata)
    {
        var callback = GCHandle.FromIntPtr(userdata).Target as Action<uint>;
        callback(trackId);
    }

    public Action<uint> OnAddTrack
    {
        set
        {
            if (onAddTrackHandle.IsAllocated)
            {
                onAddTrackHandle.Free();
            }

            onAddTrackHandle = GCHandle.Alloc(value);
            sora_set_on_add_track(p, TrackCallback, GCHandle.ToIntPtr(onAddTrackHandle));
        }
    }
    public Action<uint> OnRemoveTrack
    {
        set
        {
            if (onRemoveTrackHandle.IsAllocated)
            {
                onRemoveTrackHandle.Free();
            }

            onRemoveTrackHandle = GCHandle.Alloc(value);
            sora_set_on_remove_track(p, TrackCallback, GCHandle.ToIntPtr(onRemoveTrackHandle));
        }
    }

    private delegate void NotifyCallbackDelegate(string json, int size, IntPtr userdata);

    [AOT.MonoPInvokeCallback(typeof(NotifyCallbackDelegate))]
    static private void NotifyCallback(string json, int size, IntPtr userdata)
    {
        var callback = GCHandle.FromIntPtr(userdata).Target as Action<string>;
        callback(json);
    }

    public Action<string> OnNotify
    {
        set
        {
            if (onNotifyHandle.IsAllocated)
            {
                onNotifyHandle.Free();
            }

            onNotifyHandle = GCHandle.Alloc(value);
            sora_set_on_notify(p, NotifyCallback, GCHandle.ToIntPtr(onNotifyHandle));
        }
    }

    private delegate void PushCallbackDelegate(string json, int size, IntPtr userdata);

    [AOT.MonoPInvokeCallback(typeof(PushCallbackDelegate))]
    static private void PushCallback(string json, int size, IntPtr userdata)
    {
        var callback = GCHandle.FromIntPtr(userdata).Target as Action<string>;
        callback(json);
    }

    public Action<string> OnPush
    {
        set
        {
            if (onPushHandle.IsAllocated)
            {
                onPushHandle.Free();
            }

            onPushHandle = GCHandle.Alloc(value);
            sora_set_on_push(p, PushCallback, GCHandle.ToIntPtr(onPushHandle));
        }
    }

    public void DispatchEvents()
    {
        sora_dispatch_events(p);
    }

    public void ProcessAudio(float[] data, int offset, int samples)
    {
        sora_process_audio(p, data, offset, samples);
    }

    private delegate void HandleAudioCallbackDelegate(IntPtr buf, int samples, int channels, IntPtr userdata);

    [AOT.MonoPInvokeCallback(typeof(HandleAudioCallbackDelegate))]
    static private void HandleAudioCallback(IntPtr buf, int samples, int channels, IntPtr userdata)
    {
        var callback = GCHandle.FromIntPtr(userdata).Target as Action<short[], int, int>;
        short[] buf2 = new short[samples * channels];
        Marshal.Copy(buf, buf2, 0, samples * channels);
        callback(buf2, samples, channels);
    }

    public Action<short[], int, int> OnHandleAudio
    {
        set
        {
            if (onHandleAudioHandle.IsAllocated)
            {
                onHandleAudioHandle.Free();
            }

            onHandleAudioHandle = GCHandle.Alloc(value);
            sora_set_on_handle_audio(p, HandleAudioCallback, GCHandle.ToIntPtr(onHandleAudioHandle));
        }
    }

    private delegate void StatsCallbackDelegate(string json, int size, IntPtr userdata);

    [AOT.MonoPInvokeCallback(typeof(StatsCallbackDelegate))]
    static private void StatsCallback(string json, int size, IntPtr userdata)
    {
        GCHandle handle = GCHandle.FromIntPtr(userdata);
        var callback = GCHandle.FromIntPtr(userdata).Target as Action<string>;
        callback(json);
        handle.Free();
    }

    public void GetStats(Action<string> onGetStats)
    {
        GCHandle handle = GCHandle.Alloc(onGetStats);
        sora_get_stats(p, StatsCallback, GCHandle.ToIntPtr(handle));
    }

    public void SetAudioEnabled(bool status)
    {
        sora_set_audio_enabled(p, status ? 1 : 0);
    }

    public void SetVideoEnabled(bool status)
    {
        sora_set_video_enabled(p, status ? 1 : 0);
    }

    public bool IsAudioEnabled()
    {
        return sora_is_audio_enabled(p) != 0;
    }

    public bool IsVideoEnabled()
    {
        return sora_is_video_enabled(p) != 0;
    }

    private delegate void DeviceEnumCallbackDelegate(string device_name, string unique_name, IntPtr userdata);

    [AOT.MonoPInvokeCallback(typeof(DeviceEnumCallbackDelegate))]
    static private void DeviceEnumCallback(string device_name, string unique_name, IntPtr userdata)
    {
        var callback = GCHandle.FromIntPtr(userdata).Target as Action<string, string>;
        callback(device_name, unique_name);
    }

    public struct DeviceInfo
    {
        public string DeviceName;
        public string UniqueName;
    }
    public static DeviceInfo[] GetVideoCapturerDevices()
    {
        var list = new System.Collections.Generic.List<DeviceInfo>();
        Action<string, string> f = (deviceName, uniqueName) =>
        {
            list.Add(new DeviceInfo()
            {
                DeviceName = deviceName,
                UniqueName = uniqueName,
            });
        };

        GCHandle handle = GCHandle.Alloc(f);
        int result = sora_device_enum_video_capturer(DeviceEnumCallback, GCHandle.ToIntPtr(handle));
        handle.Free();

        if (result == 0)
        {
            return null;
        }

        return list.ToArray();
    }

    public static DeviceInfo[] GetAudioRecordingDevices()
    {
        var list = new System.Collections.Generic.List<DeviceInfo>();
        Action<string, string> f = (deviceName, uniqueName) =>
        {
            list.Add(new DeviceInfo()
            {
                DeviceName = deviceName,
                UniqueName = uniqueName,
            });
        };

        GCHandle handle = GCHandle.Alloc(f);
        int result = sora_device_enum_audio_recording(DeviceEnumCallback, GCHandle.ToIntPtr(handle));
        handle.Free();

        if (result == 0)
        {
            return null;
        }

        return list.ToArray();
    }

    public static DeviceInfo[] GetAudioPlayoutDevices()
    {
        var list = new System.Collections.Generic.List<DeviceInfo>();
        Action<string, string> f = (deviceName, uniqueName) =>
        {
            list.Add(new DeviceInfo()
            {
                DeviceName = deviceName,
                UniqueName = uniqueName,
            });
        };

        GCHandle handle = GCHandle.Alloc(f);
        int result = sora_device_enum_audio_playout(DeviceEnumCallback, GCHandle.ToIntPtr(handle));
        handle.Free();

        if (result == 0)
        {
            return null;
        }

        return list.ToArray();
    }

    public static bool IsH264Supported()
    {
        return sora_is_h264_supported() != 0;
    }

#if UNITY_IOS && !UNITY_EDITOR
    [DllImport("__Internal")]
#else
    [DllImport("SoraUnitySdk")]
#endif
    private static extern IntPtr sora_create();
#if UNITY_IOS && !UNITY_EDITOR
    [DllImport("__Internal")]
#else
    [DllImport("SoraUnitySdk")]
#endif
    private static extern void sora_set_on_add_track(IntPtr p, TrackCallbackDelegate on_add_track, IntPtr userdata);
#if UNITY_IOS && !UNITY_EDITOR
    [DllImport("__Internal")]
#else
    [DllImport("SoraUnitySdk")]
#endif
    private static extern void sora_set_on_remove_track(IntPtr p, TrackCallbackDelegate on_remove_track, IntPtr userdata);
#if UNITY_IOS && !UNITY_EDITOR
    [DllImport("__Internal")]
#else
    [DllImport("SoraUnitySdk")]
#endif
    private static extern void sora_set_on_notify(IntPtr p, NotifyCallbackDelegate on_notify, IntPtr userdata);
#if UNITY_IOS && !UNITY_EDITOR
    [DllImport("__Internal")]
#else
    [DllImport("SoraUnitySdk")]
#endif
    private static extern void sora_set_on_push(IntPtr p, PushCallbackDelegate on_push, IntPtr userdata);
#if UNITY_IOS && !UNITY_EDITOR
    [DllImport("__Internal")]
#else
    [DllImport("SoraUnitySdk")]
#endif
    private static extern void sora_dispatch_events(IntPtr p);
#if UNITY_IOS && !UNITY_EDITOR
    [DllImport("__Internal")]
#else
    [DllImport("SoraUnitySdk")]
#endif
    private static extern int sora_connect(
        IntPtr p,
        string unity_version,
        string signaling_url,
        string channel_id,
        string metadata,
        string role,
        int multistream,
        int spotlight,
        int spotlight_number,
        int simulcast,
        int capturer_type,
        IntPtr unity_camera_texture,
        string video_capturer_device,
        int video_width,
        int video_height,
        string video_codec,
        int video_bitrate,
        int unity_audio_input,
        int unity_audio_output,
        string audio_recording_device,
        string audio_playout_device,
        string audio_codec,
        int audio_bitrate,
        int data_channel_signaling,
        int data_channel_signaling_timeout,
        int ignore_disconnect_websocket,
        int close_websocket);
#if UNITY_IOS && !UNITY_EDITOR
    [DllImport("__Internal")]
#else
    [DllImport("SoraUnitySdk")]
#endif
    private static extern IntPtr sora_get_texture_update_callback();
#if UNITY_IOS && !UNITY_EDITOR
    [DllImport("__Internal")]
#else
    [DllImport("SoraUnitySdk")]
#endif
    private static extern void sora_destroy(IntPtr p);
#if UNITY_IOS && !UNITY_EDITOR
    [DllImport("__Internal")]
#else
    [DllImport("SoraUnitySdk")]
#endif
    private static extern IntPtr sora_get_render_callback();
#if UNITY_IOS && !UNITY_EDITOR
    [DllImport("__Internal")]
#else
    [DllImport("SoraUnitySdk")]
#endif
    private static extern int sora_get_render_callback_event_id(IntPtr p);
#if UNITY_IOS && !UNITY_EDITOR
    [DllImport("__Internal")]
#else
    [DllImport("SoraUnitySdk")]
#endif
    private static extern void sora_process_audio(IntPtr p, [In] float[] data, int offset, int samples);
#if UNITY_IOS && !UNITY_EDITOR
    [DllImport("__Internal")]
#else
    [DllImport("SoraUnitySdk")]
#endif
    private static extern void sora_set_on_handle_audio(IntPtr p, HandleAudioCallbackDelegate on_handle_audio, IntPtr userdata);
#if UNITY_IOS && !UNITY_EDITOR
    [DllImport("__Internal")]
#else
    [DllImport("SoraUnitySdk")]
#endif
    private static extern void sora_get_stats(IntPtr p, StatsCallbackDelegate on_get_stats, IntPtr userdata);
#if UNITY_IOS && !UNITY_EDITOR
    [DllImport("__Internal")]
#else
    [DllImport("SoraUnitySdk")]
#endif
    private static extern void sora_set_audio_enabled(IntPtr p, int status);
#if UNITY_IOS && !UNITY_EDITOR
    [DllImport("__Internal")]
#else
    [DllImport("SoraUnitySdk")]
#endif
    private static extern void sora_set_video_enabled(IntPtr p, int status);
#if UNITY_IOS && !UNITY_EDITOR
    [DllImport("__Internal")]
#else
    [DllImport("SoraUnitySdk")]
#endif
    private static extern int sora_is_audio_enabled(IntPtr p);
#if UNITY_IOS && !UNITY_EDITOR
    [DllImport("__Internal")]
#else
    [DllImport("SoraUnitySdk")]
#endif
    private static extern int sora_is_video_enabled(IntPtr p);
#if UNITY_IOS && !UNITY_EDITOR
    [DllImport("__Internal")]
#else
    [DllImport("SoraUnitySdk")]
#endif
    private static extern int sora_device_enum_video_capturer(DeviceEnumCallbackDelegate f, IntPtr userdata);
#if UNITY_IOS && !UNITY_EDITOR
    [DllImport("__Internal")]
#else
    [DllImport("SoraUnitySdk")]
#endif
    private static extern int sora_device_enum_audio_recording(DeviceEnumCallbackDelegate f, IntPtr userdata);
#if UNITY_IOS && !UNITY_EDITOR
    [DllImport("__Internal")]
#else
    [DllImport("SoraUnitySdk")]
#endif
    private static extern int sora_device_enum_audio_playout(DeviceEnumCallbackDelegate f, IntPtr userdata);
#if UNITY_IOS && !UNITY_EDITOR
    [DllImport("__Internal")]
#else
    [DllImport("SoraUnitySdk")]
#endif
    private static extern int sora_is_h264_supported();
}
