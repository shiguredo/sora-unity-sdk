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
    public class Config
    {
        public string SignalingUrl = "";
        public string ChannelId = "";
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
        public int VideoWidth = 640;
        public int VideoHeight = 480;
    }

    IntPtr p;
    GCHandle onAddTrackHandle;
    GCHandle onRemoveTrackHandle;
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

        return sora_connect(p, config.SignalingUrl, config.ChannelId, config.Role == Role.Downstream, config.Multistream, (int)config.CapturerType, unityCameraTexture, config.VideoWidth, config.VideoHeight) == 0;
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

    public void DispatchEvents()
    {
        sora_dispatch_events(p);
    }

    [DllImport("SoraUnitySdk")]
    private static extern IntPtr sora_create();
    [DllImport("SoraUnitySdk")]
    private static extern void sora_set_on_add_track(IntPtr p, TrackCallbackDelegate on_add_track, IntPtr userdata);
    [DllImport("SoraUnitySdk")]
    private static extern void sora_set_on_remove_track(IntPtr p, TrackCallbackDelegate on_remove_track, IntPtr userdata);
    [DllImport("SoraUnitySdk")]
    private static extern void sora_dispatch_events(IntPtr p);
    [DllImport("SoraUnitySdk")]
    private static extern int sora_connect(IntPtr p, string signaling_url, string channel_id, bool downstream, bool multistream, int capturer_type, IntPtr unity_camera_texture, int unity_camera_width, int unity_camera_height);
    [DllImport("SoraUnitySdk")]
    private static extern IntPtr sora_get_texture_update_callback();
    [DllImport("SoraUnitySdk")]
    private static extern void sora_destroy(IntPtr p);
    [DllImport("SoraUnitySdk")]
    private static extern IntPtr sora_get_render_callback();
    [DllImport("SoraUnitySdk")]
    private static extern int sora_get_render_callback_event_id(IntPtr p);
}
