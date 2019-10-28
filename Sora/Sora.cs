using System;
using System.Runtime.InteropServices;

public class Sora : IDisposable
{
    public enum Mode
    {
        Pubsub_Recvonly,
        Pubsub_Sendonly,
        Multistream_Recvonly,
        Multistream_Sendrecv,
    }
    public class Config
    {
        public string SignalingUrl = "";
        public string ChannelId = "";
        public Mode Mode = Sora.Mode.Pubsub_Recvonly;
    }

    IntPtr p;
    GCHandle onAddTrackHandle;
    GCHandle onRemoveTrackHandle;
    UnityEngine.Rendering.CommandBuffer commandBuffer;

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
        bool downstream;
        bool multistream;
        switch (config.Mode)
        {
            case Mode.Pubsub_Recvonly:
                downstream = true;
                multistream = false;
                break;
            case Mode.Pubsub_Sendonly:
                downstream = false;
                multistream = false;
                break;
            case Mode.Multistream_Recvonly:
                downstream = true;
                multistream = true;
                break;
            case Mode.Multistream_Sendrecv:
                downstream = false;
                multistream = true;
                break;
            default:
                return false;
        }

        return sora_connect(p, config.SignalingUrl, config.ChannelId, downstream, multistream) == 0;
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
    private static extern int sora_connect(IntPtr p, string signaling_url, string channel_id, bool downstream, bool multistream);
    [DllImport("SoraUnitySdk")]
    private static extern IntPtr sora_get_texture_update_callback();
    [DllImport("SoraUnitySdk")]
    private static extern void sora_destroy(IntPtr p);
}
