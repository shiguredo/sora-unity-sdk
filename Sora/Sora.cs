using System;
using System.Runtime.InteropServices;

public class Sora : IDisposable
{
    public enum Role
    {
        Downstream,
        Upstream,
    }
    public enum CapturerType
    {
        DeviceCamera = 0,
        UnityCamera = 1,
    }
    public enum VideoCodec {
        VP9,
        VP8,
        H264,
    }
    public enum AudioCodec {
        OPUS,
    }
    public class Config
    {
        public string SignalingUrl = "";
        public string ChannelId = "";
        public string Metadata = "";
        public Role Role = Sora.Role.Downstream;
        public bool Multistream = false;
        public void SetUnityCamera(UnityEngine.Camera camera, int width, int height)
        {
            CapturerType = Sora.CapturerType.UnityCamera;
            UnityCamera = camera;
            VideoWidth = width;
            VideoHeight = height;
        }

        public CapturerType CapturerType = Sora.CapturerType.DeviceCamera;
        public UnityEngine.Camera UnityCamera = null;
        public string VideoCapturerDevice = "";
        public int VideoWidth = 640;
        public int VideoHeight = 480;
        public VideoCodec VideoCodec = VideoCodec.VP9;
        public int VideoBitrate = 0;
        public bool UnityAudioInput = false;
        public string AudioRecordingDevice = "";
        public string AudioPlayoutDevice = "";
        public AudioCodec AudioCodec = AudioCodec.OPUS;
        public int AudioBitrate = 0;
    }

    IntPtr p;
    GCHandle onAddTrackHandle;
    GCHandle onRemoveTrackHandle;
    GCHandle onNotifyHandle;
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

        if (p != IntPtr.Zero)
        {
            sora_destroy(p);
            p = IntPtr.Zero;
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
            var texture = new UnityEngine.RenderTexture(config.VideoWidth, config.VideoHeight, 0, UnityEngine.RenderTextureFormat.BGRA32);
            unityCamera.targetTexture = texture;
            unityCamera.enabled = true;
            unityCameraTexture = texture.GetNativeTexturePtr();
        }

        return sora_connect(
            p,
            config.SignalingUrl,
            config.ChannelId,
            config.Metadata,
            config.Role == Role.Downstream,
            config.Multistream,
            (int)config.CapturerType,
            unityCameraTexture,
            config.VideoCapturerDevice,
            config.VideoWidth,
            config.VideoHeight,
            config.VideoCodec.ToString(),
            config.VideoBitrate,
            config.UnityAudioInput,
            config.AudioRecordingDevice,
            config.AudioPlayoutDevice,
            config.AudioCodec.ToString(),
            config.AudioBitrate) == 0;
    }

    public void OnRender() {
        UnityEngine.GL.IssuePluginEvent(sora_get_render_callback(), sora_get_render_callback_event_id(p));
    }

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

    public void DispatchEvents()
    {
        sora_dispatch_events(p);
    }

    public void ProcessAudio(float[] data, int offset, int samples) {
        sora_process_audio(p, data, offset, samples);
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
        bool result = sora_device_enum_video_capturer(DeviceEnumCallback, GCHandle.ToIntPtr(handle));
        handle.Free();

        if (!result)
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
        bool result = sora_device_enum_audio_recording(DeviceEnumCallback, GCHandle.ToIntPtr(handle));
        handle.Free();

        if (!result)
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
        bool result = sora_device_enum_audio_playout(DeviceEnumCallback, GCHandle.ToIntPtr(handle));
        handle.Free();

        if (!result)
        {
            return null;
        }

        return list.ToArray();
    }

    [DllImport("SoraUnitySdk")]
    private static extern IntPtr sora_create();
    [DllImport("SoraUnitySdk")]
    private static extern void sora_set_on_add_track(IntPtr p, TrackCallbackDelegate on_add_track, IntPtr userdata);
    [DllImport("SoraUnitySdk")]
    private static extern void sora_set_on_remove_track(IntPtr p, TrackCallbackDelegate on_remove_track, IntPtr userdata);
    [DllImport("SoraUnitySdk")]
    private static extern void sora_set_on_notify(IntPtr p, NotifyCallbackDelegate on_notify, IntPtr userdata);
    [DllImport("SoraUnitySdk")]
    private static extern void sora_dispatch_events(IntPtr p);
    [DllImport("SoraUnitySdk")]
    private static extern int sora_connect(
        IntPtr p,
        string signaling_url,
        string channel_id,
        string metadata,
        bool downstream,
        bool multistream,
        int capturer_type,
        IntPtr unity_camera_texture,
        string video_capturer_device,
        int video_width,
        int video_height,
        string video_codec,
        int video_bitrate,
        bool unity_audio_input,
        string audio_recording_device,
        string audio_playout_device,
        string audio_codec,
        int audio_bitrate);
    [DllImport("SoraUnitySdk")]
    private static extern IntPtr sora_get_texture_update_callback();
    [DllImport("SoraUnitySdk")]
    private static extern void sora_destroy(IntPtr p);
    [DllImport("SoraUnitySdk")]
    private static extern IntPtr sora_get_render_callback();
    [DllImport("SoraUnitySdk")]
    private static extern int sora_get_render_callback_event_id(IntPtr p);
    [DllImport("SoraUnitySdk")]
    private static extern void sora_process_audio(IntPtr p, [In] float[] data, int offset, int samples);
    [DllImport("SoraUnitySdk")]
    private static extern bool sora_device_enum_video_capturer(DeviceEnumCallbackDelegate f, IntPtr userdata);
    [DllImport("SoraUnitySdk")]
    private static extern bool sora_device_enum_audio_recording(DeviceEnumCallbackDelegate f, IntPtr userdata);
    [DllImport("SoraUnitySdk")]
    private static extern bool sora_device_enum_audio_playout(DeviceEnumCallbackDelegate f, IntPtr userdata);
}
