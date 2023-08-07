using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

public class Sora : IDisposable
{
    public enum Role
    {
        Sendonly,
        Recvonly,
        Sendrecv,
    }
    public enum Direction
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
        LYRA,
    }
    // SimulcastRid のためのパラメータ
    public enum SimulcastRidType
    {
        R0,
        R1,
        R2,
    }

    // SpotlightFocusRid と SpotlightUnfocusRid のためのパラメータ
    public enum SpotlightFocusRidType
    {
        None,
        R0,
        R1,
        R2,
    }

    public class DataChannel
    {
        // 以下は設定必須
        public string Label = "";
        public Direction Direction = Sora.Direction.Sendrecv;
        // 以下は null でない場合のみ設定される
        public bool? Ordered;
        public int? MaxPacketLifeTime;
        public int? MaxRetransmits;
        public string Protocol;
        public bool? Compress;
    }

    public const string ActionBlock = "block";
    public const string ActionAllow = "allow";
    public const string FieldConnectionId = "connection_id";
    public const string FieldClientId = "client_id";
    public const string FieldKind = "kind";
    public const string OperatorIsIn = "is_in";
    public const string OperatorIsNotIn = "is_not_in";

    public class ForwardingFilter
    {
        public string Action;
        public class Rule
        {
            public string Field;
            public string Operator;
            public List<string> Values = new List<string>();
        }
        public List<List<Rule>> Rules = new List<List<Rule>>();
    }

    public class Config
    {
        public string SignalingUrl = "";
        public string[] SignalingUrlCandidate = new string[0];
        public string ChannelId = "";
        public string ClientId = "";
        public string BundleId = "";
        public string Metadata = "";
        public string SignalingNotifyMetadata = "";
        public Role Role = Sora.Role.Sendonly;
        public bool? Multistream;
        public bool? Spotlight;
        public int SpotlightNumber = 0;
        public SpotlightFocusRidType? SpotlightFocusRid;
        public SpotlightFocusRidType? SpotlightUnfocusRid;
        public bool? Simulcast;
        public SimulcastRidType? SimulcastRid = null;
        public CapturerType CapturerType = Sora.CapturerType.DeviceCamera;
        public UnityEngine.Camera UnityCamera = null;
        public int UnityCameraRenderTargetDepthBuffer = 16;
        public bool Video = true;
        public bool Audio = true;
        public string VideoCapturerDevice = "";
        public int VideoWidth = 640;
        public int VideoHeight = 480;
        public int VideoFps = 30;
        public VideoCodecType VideoCodecType = VideoCodecType.VP9;
        public string VideoVp9Params = "";
        public string VideoAv1Params = "";
        public string VideoH264Params = "";
        public int VideoBitRate = 0;
        public bool UnityAudioInput = false;
        public bool UnityAudioOutput = false;
        public string AudioRecordingDevice = "";
        public string AudioPlayoutDevice = "";
        public AudioCodecType AudioCodecType = AudioCodecType.OPUS;
        // AudioCodecType.LYRA の場合は必須
        public int AudioCodecLyraBitrate = 0;
        public bool? AudioCodecLyraUsedtx;
        public bool CheckLyraVersion = false;
        public int AudioBitRate = 0;
        public string AudioStreamingLanguageCode = "";

        // DataChannelSignaling を有効にするかどうか
        public bool EnableDataChannelSignaling = false;
        // DataChannel を使ったシグナリングを利用するかどうか
        public bool DataChannelSignaling = false;
        // DataChannel のデータが何秒間やって来なければ切断したとみなすか
        public int DataChannelSignalingTimeout = 180;
        // IgnoreDisconnectWebsocket を有効にするかどうか
        public bool EnableIgnoreDisconnectWebsocket = false;
        // DataChannel の接続が確立した後は、WebSocket が切断されても Sora との接続を確立したままとするかどうか
        public bool IgnoreDisconnectWebsocket = false;
        // DataChannel を閉じる際に最大で何秒間待つか
        public int DisconnectWaitTimeout = 5;

        // DataChannel メッセージング
        public List<DataChannel> DataChannels = new List<DataChannel>();

        // true の場合は署名検証をしない
        public bool Insecure = false;

        // Proxy の設定
        public string ProxyUrl = "";
        public string ProxyUsername = "";
        public string ProxyPassword = "";
        // Proxy サーバーに接続するときの User-Agent。未設定ならデフォルト値が使われる
        public string ProxyAgent = "";

        public ForwardingFilter ForwardingFilter;

        // MRC の設定
        public bool? MrcHologramCompositionEnabled;
        public bool? MrcRecordingIndicatorEnabled;
        public bool? MrcVideoStabilizationEnabled;
        public uint? MrcVideoStabilizationBufferLength;
        public float? GlobalOpacityCoefficient;
    }

    IntPtr p;
    GCHandle onAddTrackHandle;
    GCHandle onRemoveTrackHandle;
    GCHandle onSetOfferHandle;
    GCHandle onNotifyHandle;
    GCHandle onPushHandle;
    GCHandle onMessageHandle;
    GCHandle onDisconnectHandle;
    GCHandle onDataChannelHandle;
    GCHandle onHandleAudioHandle;
    GCHandle onCapturerFrameHandle;
    UnityEngine.Rendering.CommandBuffer commandBuffer;
    UnityEngine.Camera unityCamera;

    public void Dispose()
    {
        if (p != IntPtr.Zero)
        {
            sora_destroy(p);
            p = IntPtr.Zero;
        }

        if (onAddTrackHandle.IsAllocated)
        {
            onAddTrackHandle.Free();
        }

        if (onRemoveTrackHandle.IsAllocated)
        {
            onRemoveTrackHandle.Free();
        }

        if (onSetOfferHandle.IsAllocated)
        {
            onSetOfferHandle.Free();
        }

        if (onNotifyHandle.IsAllocated)
        {
            onNotifyHandle.Free();
        }

        if (onPushHandle.IsAllocated)
        {
            onPushHandle.Free();
        }

        if (onMessageHandle.IsAllocated)
        {
            onMessageHandle.Free();
        }

        if (onDisconnectHandle.IsAllocated)
        {
            onDisconnectHandle.Free();
        }

        if (onDataChannelHandle.IsAllocated)
        {
            onDataChannelHandle.Free();
        }

        if (onHandleAudioHandle.IsAllocated)
        {
            onHandleAudioHandle.Free();
        }

        if (onCapturerFrameHandle.IsAllocated)
        {
            onCapturerFrameHandle.Free();
        }
    }

    public Sora()
    {
        p = sora_create();
        commandBuffer = new UnityEngine.Rendering.CommandBuffer();
    }

    public void Connect(Config config)
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

        var cc = new SoraConf.Internal.ConnectConfig();
        cc.unity_version = UnityEngine.Application.unityVersion;
        if (config.SignalingUrl.Trim().Length != 0)
        {
            cc.signaling_url.Add(config.SignalingUrl.Trim());
        }
        foreach (var url in config.SignalingUrlCandidate)
        {
            if (url.Trim().Length != 0)
            {
                cc.signaling_url.Add(url.Trim());
            }
        }
        cc.channel_id = config.ChannelId;
        cc.client_id = config.ClientId;
        cc.bundle_id = config.BundleId;
        cc.metadata = config.Metadata;
        cc.signaling_notify_metadata = config.SignalingNotifyMetadata;
        cc.role = role;
        cc.enable_multistream = config.Multistream != null;
        cc.multistream = config.Multistream == null ? false : config.Multistream.Value;
        cc.enable_spotlight = config.Spotlight != null;
        cc.spotlight = config.Spotlight == null ? false : config.Spotlight.Value;
        cc.spotlight_number = config.SpotlightNumber;
        cc.spotlight_focus_rid = config.SpotlightFocusRid == null ? "" : config.SpotlightFocusRid.Value.ToString().ToLower();
        cc.spotlight_unfocus_rid = config.SpotlightUnfocusRid == null ? "" : config.SpotlightUnfocusRid.Value.ToString().ToLower();
        cc.enable_simulcast = config.Simulcast != null;
        cc.simulcast = config.Simulcast == null ? false : config.Simulcast.Value;
        cc.simulcast_rid = config.SimulcastRid == null ? "" : config.SimulcastRid.Value.ToString().ToLower();
        cc.insecure = config.Insecure;
        cc.capturer_type = (int)config.CapturerType;
        cc.unity_camera_texture = unityCameraTexture.ToInt64();
        cc.video = config.Video;
        cc.audio = config.Audio;
        cc.video_capturer_device = config.VideoCapturerDevice;
        cc.video_width = config.VideoWidth;
        cc.video_height = config.VideoHeight;
        cc.video_fps = config.VideoFps;
        cc.video_codec_type = config.VideoCodecType.ToString();
        cc.video_vp9_params = config.VideoVp9Params;
        cc.video_av1_params = config.VideoAv1Params;
        cc.video_h264_params = config.VideoH264Params;
        cc.video_bit_rate = config.VideoBitRate;
        cc.unity_audio_input = config.UnityAudioInput;
        cc.unity_audio_output = config.UnityAudioOutput;
        cc.audio_recording_device = config.AudioRecordingDevice;
        cc.audio_playout_device = config.AudioPlayoutDevice;
        cc.audio_codec_type = config.AudioCodecType.ToString();
        cc.audio_codec_lyra_bitrate = config.AudioCodecLyraBitrate;
        cc.enable_audio_codec_lyra_usedtx = config.AudioCodecLyraUsedtx != null;
        cc.audio_codec_lyra_usedtx = config.AudioCodecLyraUsedtx == null ? false : config.AudioCodecLyraUsedtx.Value;
        cc.check_lyra_version = config.CheckLyraVersion;
        cc.audio_bit_rate = config.AudioBitRate;
        cc.audio_streaming_language_code = config.AudioStreamingLanguageCode;
        cc.enable_data_channel_signaling = config.EnableDataChannelSignaling;
        cc.data_channel_signaling = config.DataChannelSignaling;
        cc.data_channel_signaling_timeout = config.DataChannelSignalingTimeout;
        cc.enable_ignore_disconnect_websocket = config.EnableIgnoreDisconnectWebsocket;
        cc.ignore_disconnect_websocket = config.IgnoreDisconnectWebsocket;
        cc.disconnect_wait_timeout = config.DisconnectWaitTimeout;
        foreach (var m in config.DataChannels)
        {
            var direction =
                m.Direction == Direction.Sendonly ? "sendonly" :
                m.Direction == Direction.Recvonly ? "recvonly" : "sendrecv";
            var c = new SoraConf.Internal.DataChannel()
            {
                label = m.Label,
                direction = direction,
            };
            if (m.Ordered != null)
            {
                c.enable_ordered = true;
                c.ordered = m.Ordered.Value;
            }
            if (m.MaxPacketLifeTime != null)
            {
                c.enable_max_packet_life_time = true;
                c.max_packet_life_time = m.MaxPacketLifeTime.Value;
            }
            if (m.MaxRetransmits != null)
            {
                c.enable_max_retransmits = true;
                c.max_retransmits = m.MaxRetransmits.Value;
            }
            if (m.Protocol != null)
            {
                c.enable_protocol = true;
                c.protocol = m.Protocol;
            }
            if (m.Compress != null)
            {
                c.enable_compress = true;
                c.compress = m.Compress.Value;
            }
            cc.data_channels.Add(c);
        }
        cc.proxy_url = config.ProxyUrl;
        cc.proxy_username = config.ProxyUsername;
        cc.proxy_password = config.ProxyPassword;
        cc.proxy_agent = config.ProxyAgent;
        if (config.ForwardingFilter != null)
        {
            cc.enable_forwarding_filter = true;
            cc.forwarding_filter.action = config.ForwardingFilter.Action;
            foreach (var rs in config.ForwardingFilter.Rules)
            {
                var ccrs = new SoraConf.Internal.ForwardingFilter.Rules();
                foreach (var r in rs)
                {
                    var ccr = new SoraConf.Internal.ForwardingFilter.Rule();
                    ccr.field = r.Field;
                    ccr.op = r.Operator;
                    foreach (var v in r.Values)
                    {
                        ccr.values.Add(v);
                    }
                    ccrs.rules.Add(ccr);
                }
                cc.forwarding_filter.rules.Add(ccrs);
            }
        }

        if (config.MrcHologramCompositionEnabled != null)
        {
            cc.enable_mrc_hologram_composition_enabled = true;
            cc.mrc_hologram_composition_enabled = config.MrcHologramCompositionEnabled.Value;
        }
        if (config.MrcRecordingIndicatorEnabled != null)
        {
            cc.enable_mrc_recording_indicator_enabled = true;
            cc.mrc_recording_indicator_enabled = config.MrcRecordingIndicatorEnabled.Value;
        }
        if (config.MrcVideoStabilizationEnabled != null)
        {
            cc.enable_mrc_video_stabilization_enabled = true;
            cc.mrc_video_stabilization_enabled = config.MrcVideoStabilizationEnabled.Value;
        }
        if (config.MrcVideoStabilizationBufferLength != null)
        {
            cc.enable_mrc_video_stabilization_buffer_length = true;
            cc.mrc_video_stabilization_buffer_length = config.MrcVideoStabilizationBufferLength.Value;
        }
        if (config.GlobalOpacityCoefficient != null)
        {
            cc.enable_mrc_global_opacity_coefficient = true;
            cc.mrc_global_opacity_coefficient = config.GlobalOpacityCoefficient.Value;
        }

        sora_connect(p, Jsonif.Json.ToJson(cc));
    }
    public void Disconnect()
    {
        sora_disconnect(p);
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

    private delegate void TrackCallbackDelegate(uint track_id, string connection_id, IntPtr userdata);

    [AOT.MonoPInvokeCallback(typeof(TrackCallbackDelegate))]
    static private void TrackCallback(uint trackId, string connectionId, IntPtr userdata)
    {
        var callback = GCHandle.FromIntPtr(userdata).Target as Action<uint, string>;
        callback(trackId, connectionId);
    }

    public Action<uint, string> OnAddTrack
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
    public Action<uint, string> OnRemoveTrack
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

    private delegate void SetOfferCallbackDelegate(string json, IntPtr userdata);

    [AOT.MonoPInvokeCallback(typeof(SetOfferCallbackDelegate))]
    static private void SetOfferCallback(string json, IntPtr userdata)
    {
        var callback = GCHandle.FromIntPtr(userdata).Target as Action<string>;
        callback(json);
    }

    public Action<string> OnSetOffer
    {
        set
        {
            if (onSetOfferHandle.IsAllocated)
            {
                onSetOfferHandle.Free();
            }

            onSetOfferHandle = GCHandle.Alloc(value);
            sora_set_on_set_offer(p, SetOfferCallback, GCHandle.ToIntPtr(onSetOfferHandle));
        }
    }

    private delegate void NotifyCallbackDelegate(string json, IntPtr userdata);

    [AOT.MonoPInvokeCallback(typeof(NotifyCallbackDelegate))]
    static private void NotifyCallback(string json, IntPtr userdata)
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

    private delegate void PushCallbackDelegate(string json, IntPtr userdata);

    [AOT.MonoPInvokeCallback(typeof(PushCallbackDelegate))]
    static private void PushCallback(string json, IntPtr userdata)
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

    private delegate void MessageCallbackDelegate(string label, IntPtr buf, int size, IntPtr userdata);

    [AOT.MonoPInvokeCallback(typeof(MessageCallbackDelegate))]
    static private void MessageCallback(string label, IntPtr buf, int size, IntPtr userdata)
    {
        var callback = GCHandle.FromIntPtr(userdata).Target as Action<string, byte[]>;
        byte[] data = new byte[size];
        Marshal.Copy(buf, data, 0, size);
        callback(label, data);
    }

    public Action<string, byte[]> OnMessage
    {
        set
        {
            if (onMessageHandle.IsAllocated)
            {
                onMessageHandle.Free();
            }

            onMessageHandle = GCHandle.Alloc(value);
            sora_set_on_message(p, MessageCallback, GCHandle.ToIntPtr(onMessageHandle));
        }
    }

    private delegate void DisconnectCallbackDelegate(int errorCode, string message, IntPtr userdata);

    [AOT.MonoPInvokeCallback(typeof(DisconnectCallbackDelegate))]
    static private void DisconnectCallback(int errorCode, string message, IntPtr userdata)
    {
        var callback = GCHandle.FromIntPtr(userdata).Target as Action<SoraConf.ErrorCode, string>;
        callback((SoraConf.ErrorCode)errorCode, message);
    }

    public Action<SoraConf.ErrorCode, string> OnDisconnect
    {
        set
        {
            if (onDisconnectHandle.IsAllocated)
            {
                onDisconnectHandle.Free();
            }

            onDisconnectHandle = GCHandle.Alloc(value);
            sora_set_on_disconnect(p, DisconnectCallback, GCHandle.ToIntPtr(onDisconnectHandle));
        }
    }

    private delegate void DataChannelCallbackDelegate(string label, IntPtr userdata);

    [AOT.MonoPInvokeCallback(typeof(DataChannelCallbackDelegate))]
    static private void DataChannelCallback(string label, IntPtr userdata)
    {
        var callback = GCHandle.FromIntPtr(userdata).Target as Action<string>;
        callback(label);
    }

    public Action<string> OnDataChannel
    {
        set
        {
            if (onDataChannelHandle.IsAllocated)
            {
                onDataChannelHandle.Free();
            }

            onDataChannelHandle = GCHandle.Alloc(value);
            sora_set_on_data_channel(p, DataChannelCallback, GCHandle.ToIntPtr(onDataChannelHandle));
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

    private delegate void CapturerFrameCallbackDelegate(string data, IntPtr userdata);

    [AOT.MonoPInvokeCallback(typeof(CapturerFrameCallbackDelegate))]
    static private void CapturerFrameCallback(string data, IntPtr userdata)
    {
        UnityEngine.Debug.LogFormat("CapturerFrameCallback: json={0}", data);
        var callback = GCHandle.FromIntPtr(userdata).Target as Action<SoraConf.VideoFrame>;
        var frame = Jsonif.Json.FromJson<SoraConf.VideoFrame>(data);
        callback(frame);
    }

    public Action<SoraConf.VideoFrame> OnCapturerFrame
    {
        set
        {
            if (onCapturerFrameHandle.IsAllocated)
            {
                onCapturerFrameHandle.Free();
            }

            onCapturerFrameHandle = GCHandle.Alloc(value);
            sora_set_on_capturer_frame(p, CapturerFrameCallback, GCHandle.ToIntPtr(onCapturerFrameHandle));
        }
    }

    private delegate void StatsCallbackDelegate(string json, IntPtr userdata);

    [AOT.MonoPInvokeCallback(typeof(StatsCallbackDelegate))]
    static private void StatsCallback(string json, IntPtr userdata)
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

    public void SendMessage(string label, byte[] buf)
    {
        sora_send_message(p, label, buf, buf.Length);
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

    public static void Setenv(string name, string value)
    {
        sora_setenv(name, value);
    }

    public bool AudioEnabled
    {
        get { return sora_get_audio_enabled(p) != 0; }
        set { sora_set_audio_enabled(p, value ? 1 : 0); }
    }

    public bool VideoEnabled
    {
        get { return sora_get_video_enabled(p) != 0; }
        set { sora_set_video_enabled(p, value ? 1 : 0); }
    }

#if UNITY_IOS && !UNITY_EDITOR
    private const string DllName = "__Internal";
#else
    private const string DllName = "SoraUnitySdk";
#endif

    [DllImport(DllName)]
    private static extern IntPtr sora_create();
    [DllImport(DllName)]
    private static extern void sora_set_on_add_track(IntPtr p, TrackCallbackDelegate on_add_track, IntPtr userdata);
    [DllImport(DllName)]
    private static extern void sora_set_on_remove_track(IntPtr p, TrackCallbackDelegate on_remove_track, IntPtr userdata);
    [DllImport(DllName)]
    private static extern void sora_set_on_set_offer(IntPtr p, SetOfferCallbackDelegate on_set_offer, IntPtr userdata);
    [DllImport(DllName)]
    private static extern void sora_set_on_notify(IntPtr p, NotifyCallbackDelegate on_notify, IntPtr userdata);
    [DllImport(DllName)]
    private static extern void sora_set_on_push(IntPtr p, PushCallbackDelegate on_push, IntPtr userdata);
    [DllImport(DllName)]
    private static extern void sora_set_on_message(IntPtr p, MessageCallbackDelegate on_message, IntPtr userdata);
    [DllImport(DllName)]
    private static extern void sora_set_on_disconnect(IntPtr p, DisconnectCallbackDelegate on_disconnect, IntPtr userdata);
    [DllImport(DllName)]
    private static extern void sora_set_on_data_channel(IntPtr p, DataChannelCallbackDelegate on_data_channel, IntPtr userdata);
    [DllImport(DllName)]
    private static extern void sora_dispatch_events(IntPtr p);
    [DllImport(DllName)]
    private static extern void sora_connect(IntPtr p, string config);
    [DllImport(DllName)]
    private static extern void sora_disconnect(IntPtr p);
    [DllImport(DllName)]
    private static extern IntPtr sora_get_texture_update_callback();
    [DllImport(DllName)]
    private static extern void sora_destroy(IntPtr p);
    [DllImport(DllName)]
    private static extern IntPtr sora_get_render_callback();
    [DllImport(DllName)]
    private static extern int sora_get_render_callback_event_id(IntPtr p);
    [DllImport(DllName)]
    private static extern void sora_process_audio(IntPtr p, [In] float[] data, int offset, int samples);
    [DllImport(DllName)]
    private static extern void sora_set_on_handle_audio(IntPtr p, HandleAudioCallbackDelegate on_handle_audio, IntPtr userdata);
    [DllImport(DllName)]
    private static extern void sora_set_on_capturer_frame(IntPtr p, CapturerFrameCallbackDelegate on_capturer_frame, IntPtr userdata);
    [DllImport(DllName)]
    private static extern void sora_get_stats(IntPtr p, StatsCallbackDelegate on_get_stats, IntPtr userdata);
    [DllImport(DllName)]
    private static extern void sora_send_message(IntPtr p, string label, [In] byte[] buf, int size);
    [DllImport(DllName)]
    private static extern int sora_device_enum_video_capturer(DeviceEnumCallbackDelegate f, IntPtr userdata);
    [DllImport(DllName)]
    private static extern int sora_device_enum_audio_recording(DeviceEnumCallbackDelegate f, IntPtr userdata);
    [DllImport(DllName)]
    private static extern int sora_device_enum_audio_playout(DeviceEnumCallbackDelegate f, IntPtr userdata);
    [DllImport(DllName)]
    private static extern int sora_is_h264_supported();
    [DllImport(DllName)]
    private static extern void sora_setenv(string name, string value);
    [DllImport(DllName)]
    private static extern int sora_get_audio_enabled(IntPtr p);
    [DllImport(DllName)]
    private static extern void sora_set_audio_enabled(IntPtr p, int enabled);
    [DllImport(DllName)]
    private static extern int sora_get_video_enabled(IntPtr p);
    [DllImport(DllName)]
    private static extern void sora_set_video_enabled(IntPtr p, int enabled);
}
