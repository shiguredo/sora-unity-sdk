using System;
using System.Runtime.InteropServices;

public class Sora : IDisposable
{
    public class Config
    {
        public string SignalingUrl = "";
        public string ChannelId = "";
        public bool Downstream = false;
        public bool Multistream = false;
    }

    IntPtr p;
    GCHandle onAddTrackHandle;
    GCHandle onRemoveTrackHandle;

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
    }

    public int Connect(Config config)
    {
        return sora_connect(p, config.SignalingUrl, config.ChannelId, config.Downstream, config.Multistream);
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

    public IntPtr GetTextureUpdateCallback()
    {
        return sora_get_texture_update_callback();
    }

    public void DispatchEvents()
    {
        sora_dispatch_events(p);
    }

    [DllImport("sora_unity")]
    private static extern IntPtr sora_create();
    [DllImport("sora_unity")]
    private static extern void sora_set_on_add_track(IntPtr p, TrackCallbackDelegate on_add_track, IntPtr userdata);
    [DllImport("sora_unity")]
    private static extern void sora_set_on_remove_track(IntPtr p, TrackCallbackDelegate on_remove_track, IntPtr userdata);
    [DllImport("sora_unity")]
    private static extern void sora_dispatch_events(IntPtr p);
    [DllImport("sora_unity")]
    private static extern int sora_connect(IntPtr p, string signaling_url, string channel_id, bool downstream, bool multistream);
    [DllImport("sora_unity")]
    private static extern IntPtr sora_get_texture_update_callback();
    [DllImport("sora_unity")]
    private static extern void sora_destroy(IntPtr p);
}
