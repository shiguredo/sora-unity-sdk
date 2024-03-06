using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEngine;

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
        H265,
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
        public string? Protocol;
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
        public string? Action;
        public class Rule
        {
            public string Field;
            public string Operator;
            public List<string> Values = new List<string>();
        }
        public List<List<Rule>> Rules = new List<List<Rule>>();
        public string? Version;
        public string? Metadata;
    }

    /// <summary>
    /// カメラの設定
    /// </summary>
    /// <remarks>
    /// デバイスカメラまたは Unity カメラの設定を行います。
    /// 各フィールドに直接値を入力しても構いませんが、FromDeviceCamera() または FromUnityCamera() を使って生成することを推奨します。
    /// 
    /// Sora.GetVideoCapturerDevices() で取得した DeviceName または UniqueName を VideoCapturerDevice に設定することで、
    /// 指定したデバイスのカメラを利用することが出来ます。
    /// </remarks>
    public class CameraConfig
    {
        public CapturerType CapturerType = Sora.CapturerType.DeviceCamera;
        public UnityEngine.Camera UnityCamera = null;
        public int UnityCameraRenderTargetDepthBuffer = 16;
        // 空文字ならデフォルトデバイスを利用する
        public string VideoCapturerDevice = "";
        public int VideoWidth = 640;
        public int VideoHeight = 480;
        public int VideoFps = 30;

        public static CameraConfig FromUnityCamera(UnityEngine.Camera unityCamera, int unityCameraRenderTargetDepthBuffer, int videoWidth, int videoHeight, int videoFps)
        {
            return new CameraConfig()
            {
                CapturerType = Sora.CapturerType.UnityCamera,
                UnityCamera = unityCamera,
                UnityCameraRenderTargetDepthBuffer = unityCameraRenderTargetDepthBuffer,
                VideoCapturerDevice = "",
                VideoWidth = videoWidth,
                VideoHeight = videoHeight,
                VideoFps = videoFps,
            };
        }
        public static CameraConfig FromDeviceCamera(string videoCapturerDevice, int videoWidth, int videoHeight, int videoFps)
        {
            return new CameraConfig()
            {
                CapturerType = Sora.CapturerType.DeviceCamera,
                UnityCamera = null,
                UnityCameraRenderTargetDepthBuffer = 16,
                VideoCapturerDevice = videoCapturerDevice,
                VideoWidth = videoWidth,
                VideoHeight = videoHeight,
                VideoFps = videoFps,
            };
        }
    }

    /// <summary>
    /// Sora の設定
    /// </summary>
    /// <remarks>
    /// Sora オブジェクトに渡す設定です。
    /// 多くのフィールドは WebRTC SFU Sora に接続する時のパラメータと同じなので、基本的には WebRTC SFU Sora ドキュメントの
    /// <see href="https://sora-doc.shiguredo.jp/SIGNALING/">WebSocket 経由のシグナリング</see> を参照してください。
    /// </remarks>
    public class Config
    {
        /// <summary>
        /// シグナリング URL
        /// </summary>
        /// <remarks>
        /// SignalingUrlCandidate を使って複数の URL を指定することもできます。
        /// 複数の URL が指定されている場合、すべての URL に接続を試み、最初に接続が確立した URL だけ残して、他の URL は切断します。
        /// SignalingUrl と SignalingUrlCandidate の両方に値が設定されている場合、両方の URL を使って接続を試みます。
        /// </remarks>
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
        // NoVideoDevice と NoAudioDevice を true にすると、ビデオデバイスまたはオーディオデバイスを掴まなくなります。
        // Video, Audio を false にすると、ビデオやオーディオデータの送信は行わなくなりますが、依然としてデバイスそのものは掴んでしまうので、
        // もしデバイスを掴まないようにしたい場合はこれらのフィールドを true にしてください。
        // また、この設定はデバイスのみに作用するものなので、
        // Unity カメラや Unity オーディオを利用する場合は NoVideoDevice や NoAudioDevice の設定は効果がありません。
        public bool NoVideoDevice = false;
        public bool NoAudioDevice = false;
        public CameraConfig CameraConfig = new CameraConfig();
        public bool Video = true;
        public bool Audio = true;
        public VideoCodecType VideoCodecType = VideoCodecType.VP9;
        public string VideoVp9Params = "";
        public string VideoAv1Params = "";
        public string VideoH264Params = "";
        public int VideoBitRate = 0;
        // デバイスから録音する代わりに Sora.ProcessAudio() で指定したデータを録音データとして利用するかどうか
        public bool UnityAudioInput = false;
        // 再生データをデバイスで再生する代わりに Sora.OnHandleAudio コールバックで再生データを受け取るようにするかどうか
        public bool UnityAudioOutput = false;
        /// <summary>
        /// 録音時のデバイス名
        /// </summary>
        /// <remarks>
        /// 空文字ならデフォルトデバイスを利用します。
        /// Sora.GetAudioRecordingDevices() で取得した DeviceName または UniqueName を AudioRecordingDevice に設定することで、
        /// 指定したデバイスのマイクを利用することが出来ます。
        /// </remarks>
        public string AudioRecordingDevice = "";
        /// <summary>
        /// 再生時のデバイス名
        /// </summary>
        /// <remarks>
        /// 空文字ならデフォルトデバイスを利用します。
        /// SoraGetAudioPlayoutDevices() で取得した DeviceName または UniqueName を AudioPlayoutDevice に設定することで、
        /// 指定したデバイスのスピーカーを利用することが出来ます。
        /// </remarks>
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

        // ハードウェアエンコーダー/デコーダーを利用するかどうか。null の場合は実装依存となる
        public bool? UseHardwareEncoder;
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

    /// <summary>
    /// Sora オブジェクトを破棄します。
    /// </summary>
    /// <remarks>
    /// 既に Connect() を呼び出している場合、この関数を呼び出す前に、OnDisconnect コールバックが呼ばれていることを確認して下さい。
    /// OnDisconnect コールバックが呼ばれる前に Dispose() を呼び出した場合の動作は未定義です。
    /// </remarks>
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

    /// <summary>
    /// Sora に接続します。
    /// </summary>
    /// <remarks>
    /// この関数を呼び出した後は、エラー時や切断時に必ず１回だけ OnDisconnect コールバックが呼ばれます。
    /// </remarks>
    public void Connect(Config config)
    {
        IntPtr unityCameraTexture = IntPtr.Zero;
        if (config.CameraConfig.CapturerType == CapturerType.UnityCamera)
        {
            unityCamera = config.CameraConfig.UnityCamera;
            var texture = new UnityEngine.RenderTexture(config.CameraConfig.VideoWidth, config.CameraConfig.VideoHeight, config.CameraConfig.UnityCameraRenderTargetDepthBuffer, UnityEngine.RenderTextureFormat.BGRA32);
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
        cc.multistream = config.Multistream.GetValueOrDefault();
        cc.enable_spotlight = config.Spotlight != null;
        cc.spotlight = config.Spotlight.GetValueOrDefault();
        cc.spotlight_number = config.SpotlightNumber;
        cc.spotlight_focus_rid = config.SpotlightFocusRid == null ? "" : config.SpotlightFocusRid.Value.ToString().ToLower();
        cc.spotlight_unfocus_rid = config.SpotlightUnfocusRid == null ? "" : config.SpotlightUnfocusRid.Value.ToString().ToLower();
        cc.enable_simulcast = config.Simulcast != null;
        cc.simulcast = config.Simulcast.GetValueOrDefault();
        cc.simulcast_rid = config.SimulcastRid == null ? "" : config.SimulcastRid.Value.ToString().ToLower();
        cc.insecure = config.Insecure;
        cc.no_video_device = config.NoVideoDevice;
        cc.no_audio_device = config.NoAudioDevice;
        cc.video = config.Video;
        cc.audio = config.Audio;
        cc.camera_config.capturer_type = (int)config.CameraConfig.CapturerType;
        cc.camera_config.unity_camera_texture = unityCameraTexture.ToInt64();
        cc.camera_config.video_capturer_device = config.CameraConfig.VideoCapturerDevice;
        cc.camera_config.video_width = config.CameraConfig.VideoWidth;
        cc.camera_config.video_height = config.CameraConfig.VideoHeight;
        cc.camera_config.video_fps = config.CameraConfig.VideoFps;
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
        cc.audio_codec_lyra_usedtx = config.AudioCodecLyraUsedtx.GetValueOrDefault();
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
            if (config.ForwardingFilter.Action != null)
            {
                cc.forwarding_filter.enable_action = true;
                cc.forwarding_filter.action = config.ForwardingFilter.Action;
            }
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
            if (config.ForwardingFilter.Version != null)
            {
                cc.forwarding_filter.enable_version = true;
                cc.forwarding_filter.version = config.ForwardingFilter.Version;
            }
            if (config.ForwardingFilter.Metadata != null)
            {
                cc.forwarding_filter.enable_metadata = true;
                cc.forwarding_filter.metadata = config.ForwardingFilter.Metadata;
            }
        }
        cc.enable_use_hardware_encoder = config.UseHardwareEncoder != null;
        cc.use_hardware_encoder = config.UseHardwareEncoder.GetValueOrDefault();

        sora_connect(p, Jsonif.Json.ToJson(cc));
    }
    /// <summary>
    /// Sora から切断します。
    /// </summary>
    /// <remarks>
    /// この関数は何度呼び出しても構いませんが、既に切断されている場合 OnDisconnect コールバックは呼ばれません。
    /// </remarks>
    public void Disconnect()
    {
        sora_disconnect(p);
    }

    /// <summary>
    /// カメラを切り替えます。
    /// </summary>
    /// <remarks>
    /// 接続中のカメラを別のカメラに切り替えます。
    /// この時、NoVideoDevice の設定は無視されます。
    /// つまり NoVideoDevice = false かつ Video = true で接続した後に SwitchCamera() を呼び出すと、指定したカメラに切り替わります。
    /// </remarks>
    public void SwitchCamera(CameraConfig config)
    {
        if (unityCamera != null)
        {
            unityCamera.enabled = false;
            unityCamera.targetTexture = null;
            unityCamera = null;
        }

        IntPtr unityCameraTexture = IntPtr.Zero;
        if (config.CapturerType == CapturerType.UnityCamera)
        {
            unityCamera = config.UnityCamera;
            var texture = new UnityEngine.RenderTexture(config.VideoWidth, config.VideoHeight, config.UnityCameraRenderTargetDepthBuffer, UnityEngine.RenderTextureFormat.BGRA32);
            unityCamera.targetTexture = texture;
            unityCamera.enabled = true;
            unityCameraTexture = texture.GetNativeTexturePtr();
        }
        var cc = new SoraConf.Internal.CameraConfig();
        cc.capturer_type = (int)config.CapturerType;
        cc.unity_camera_texture = unityCameraTexture.ToInt64();
        cc.video_capturer_device = config.VideoCapturerDevice;
        cc.video_width = config.VideoWidth;
        cc.video_height = config.VideoHeight;
        cc.video_fps = config.VideoFps;
        sora_switch_camera(p, Jsonif.Json.ToJson(cc));
    }

    /// <summary>
    /// 指定した Unity カメラの映像をキャプチャする
    /// </summary>
    /// <remarks>
    /// Unity カメラから１フレーム分の映像をキャプチャし、Sora に送信します。
    /// この関数は Unity 側でレンダリングが完了した後（yield return new WaitForEndOfFrame() の後）に呼び出して下さい。
    /// </remarks>
    public void OnRender()
    {
        UnityEngine.GL.IssuePluginEvent(sora_get_render_callback(), sora_get_render_callback_event_id(p));
    }

    /// <summary>
    /// trackId で受信した映像を texture にレンダリングする
    /// </summary>
    /// <remarks>
    /// Role.Sendonly または Role.Sendrecv で映像を送信している場合、
    /// 自身の trackId を指定すれば、自身が送信している映像を texture にレンダリングすることもできます。
    /// </remarks>
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

    /// <summary>
    /// トラックが追加された時のコールバック
    /// </summary>
    /// <remarks>
    /// Action の引数は uint trackId, string connectionId です。
    /// connectionId が空文字だった場合、送信者自身のトラックが追加されたことを表します。
    /// </remarks>
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

    /// <summary>
    /// Sora クラスから発生したイベントを処理します。
    /// </summary>
    /// <remarks>
    /// OnAddTrack や OnRemoveTrack, OnDisconnect といったコールバックは、一部を除き、この関数を呼び出すことで発生します。
    /// そのため、この関数は Update() などの定期的に実行される場所で呼び出すようにして下さい。
    ///
    /// 例外として、OnHandleAudio と OnCapturerFrame はこの関数を呼ばなくても発生しますが、
    /// Unity スレッドとは別のスレッドから呼び出されるため同期に注意する必要があります。
    /// </remarks>
    public void DispatchEvents()
    {
        sora_dispatch_events(p);
    }

    /// <summary>
    /// 録音データを処理します。
    /// </summary>
    /// <remarks>
    /// Config.UnityAudioInput = true の場合のみ有効です。
    /// 録音デバイスからの入力の代わりに、この ProcessAudio() で渡したデータを入力として扱うことが出来ます。
    /// 現在は 48000Hz Stereo のデータのみ受け取ることができます。
    /// </remarks>
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

    /// <summary>
    /// オーディオ再生用のコールバック
    /// </summary>
    /// <remarks>
    /// Config.UnityAudioOutput = true の場合のみ有効です。
    /// 再生デバイスで直接再生する代わりに、再生するためのデータをこのコールバックに渡します。
    /// また、このコールバックは Unity スレッドとは別のスレッドから呼ばれることに注意して下さい。
    /// </remarks>
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
        var callback = GCHandle.FromIntPtr(userdata).Target as Action<SoraConf.VideoFrame>;
        var frame = Jsonif.Json.FromJson<SoraConf.VideoFrame>(data);
        callback(frame);
    }

    /// <summary>
    /// カメラからの映像をキャプチャする際のコールバック
    /// </summary>
    /// <remarks>
    /// RenderTrackToTexture() でもカメラからの映像データを取得することはできますが、
    /// ビットレートに応じて縮小された後のデータになってしまいます。
    /// 
    /// OnCapturerFrame でコールバックされるのは、縮小される前の、カメラから取得した直後のサイズのデータになります。
    /// 高画質な映像を取得したい場合は、このコールバックを利用してください。
    /// 
    /// このコールバックは Unity スレッドとは別のスレッドから呼ばれることに注意して下さい。
    /// また、Android ではこのコールバックを利用できません。
    /// </remarks>
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

    /// <summary>
    /// 指定した label のデータチャンネルに buf を送信します。
    /// </summary>
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

    /// <summary>
    /// 候補の中から実際に接続されたシグナリング URL
    /// </summary>
    /// <remarks>
    /// Sora の接続時に指定したシグナリング URL の中から、どのシグナリング URL に接続したのかを返します。
    /// 必ず Config.SignalingUrl または Config.SignalingUrlCandidate で指定された値のいずれかを返します。
    /// </remarks>
    public string SelectedSignalingURL
    {
        get
        {
            int size = sora_get_selected_signaling_url_size(p);
            byte[] buf = new byte[size];
            sora_get_selected_signaling_url(p, buf, size);
            return System.Text.Encoding.UTF8.GetString(buf);
        }
    }

    /// <summary>
    /// 最終的に接続されたシグナリング URL
    /// </summary>
    /// <remarks>
    /// Sora でクラスターを組んでいる場合は、リダイレクトによって別のシグナリング URL に接続することがあります。
    /// この関数はリダイレクト時に指定されたシグナリング URL を返します。
    /// この値は Config.SignalingUrl または Config.SignalingUrlCandidate で指定された値とは異なる場合があります。
    /// </remarks>
    public string ConnectedSignalingURL
    {
        get
        {
            int size = sora_get_connected_signaling_url_size(p);
            byte[] buf = new byte[size];
            sora_get_connected_signaling_url(p, buf, size);
            return System.Text.Encoding.UTF8.GetString(buf);
        }
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
    private static extern void sora_switch_camera(IntPtr p, string config);
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
    [DllImport(DllName)]
    private static extern int sora_get_selected_signaling_url_size(IntPtr p);
    [DllImport(DllName)]
    private static extern int sora_get_connected_signaling_url_size(IntPtr p);
    [DllImport(DllName)]
    private static extern void sora_get_selected_signaling_url(IntPtr p, [Out] byte[] buf, int size);
    [DllImport(DllName)]
    private static extern void sora_get_connected_signaling_url(IntPtr p, [Out] byte[] buf, int size);

    public interface IAudioOutputHelper : IDisposable
    {
        bool IsHandsfree();
        void SetHandsfree(bool enabled);
    }

#if UNITY_ANDROID && !UNITY_EDITOR
    // Androidの実装
    public class AndroidAudioOutputHelper : IAudioOutputHelper
    {
        private AndroidJavaObject soraAudioManager;
        private ChangeRouteCallbackProxy callbackProxy;

        private class ChangeRouteCallbackProxy : AndroidJavaProxy
        {
            public event Action onChangeRoute;

            public ChangeRouteCallbackProxy() : base("jp.shiguredo.sora.audiomanager.SoraAudioManager$OnChangeRouteObserver")
            {
            }

            public void OnChangeRoute()
            {
                onChangeRoute?.Invoke();
            }
        }

        public AndroidAudioOutputHelper(Action onChangeRoute)
        {
            using (var soraAudioManagerFactoryClass = new AndroidJavaClass("jp.shiguredo.sora.audiomanager.SoraAudioManagerFactory"))
            {
                AndroidJavaClass unityPlayerClass = new AndroidJavaClass("com.unity3d.player.UnityPlayer");
                AndroidJavaObject currentActivity = unityPlayerClass.GetStatic<AndroidJavaObject>("currentActivity");
                soraAudioManager = soraAudioManagerFactoryClass.CallStatic<AndroidJavaObject>("createWithMainThreadWrapper", currentActivity);

                if (onChangeRoute != null)
                {
                    callbackProxy = new ChangeRouteCallbackProxy();
                    callbackProxy.onChangeRoute += onChangeRoute;
                }
                soraAudioManager.Call("start", callbackProxy);
            }
        }

        public void Dispose()
        {
            soraAudioManager.Call("stop");
            soraAudioManager = null;
            callbackProxy = null;
        }

        public bool IsHandsfree()
        {
            return soraAudioManager.Call<bool>("isHandsfree");
        }

        public void SetHandsfree(bool enabled)
        {
            soraAudioManager.Call("setHandsfree", enabled);
        }
    }
#endif

    // 他のプラットフォームの実装
    public class DefaultAudioOutputHelper : IAudioOutputHelper
    {
        private delegate void ChangeRouteCallbackDelegate(IntPtr userdata);

        [AOT.MonoPInvokeCallback(typeof(ChangeRouteCallbackDelegate))]
        static private void ChangeRouteCallback(IntPtr userdata)
        {
            var callback = GCHandle.FromIntPtr(userdata).Target as Action;
            callback();
        }

        GCHandle onChangeRouteHandle;
        IntPtr p;

        public DefaultAudioOutputHelper(Action onChangeRoute)
        {
            if (onChangeRoute == null)
            {
                onChangeRoute = () => {}; // 空のアクションを割り当てる
            }

            onChangeRouteHandle = GCHandle.Alloc(onChangeRoute);
            p = sora_audio_output_helper_create(ChangeRouteCallback, GCHandle.ToIntPtr(onChangeRouteHandle));
        }

        public void Dispose()
        {
            if (p != IntPtr.Zero)
            {
                sora_audio_output_helper_destroy(p);
                if (onChangeRouteHandle.IsAllocated)
                {
                    onChangeRouteHandle.Free();
                }
                p = IntPtr.Zero;
            }
        }

        public bool IsHandsfree()
        {
            return sora_audio_output_helper_is_handsfree(p) != 0;
        }

        public void SetHandsfree(bool enabled)
        {
            sora_audio_output_helper_set_handsfree(p, enabled ? 1 : 0);
        }

#if UNITY_IOS && !UNITY_EDITOR
        private const string DllName = "__Internal";
#else
        private const string DllName = "SoraUnitySdk";
#endif

        [DllImport(DllName)]
        private static extern IntPtr sora_audio_output_helper_create(ChangeRouteCallbackDelegate cb, IntPtr userdata);
        [DllImport(DllName)]
        private static extern void sora_audio_output_helper_destroy(IntPtr p);
        [DllImport(DllName)]
        private static extern int sora_audio_output_helper_is_handsfree(IntPtr p);
        [DllImport(DllName)]
        private static extern void sora_audio_output_helper_set_handsfree(IntPtr p, int enabled);
    }

    public class AudioOutputHelperFactory
    {
        public static IAudioOutputHelper Create(Action onChangeRoute)
        {
#if UNITY_ANDROID && !UNITY_EDITOR
            return new AndroidAudioOutputHelper(onChangeRoute);
#else
            return new DefaultAudioOutputHelper(onChangeRoute);
#endif
        }
    }
}
