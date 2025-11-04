using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEngine;

#nullable enable

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
        Texture = 2,
    }
    public enum VideoCodecType
    {
        VP9,
        VP8,
        H264,
        AV1,
        H265,
    }
    static string VideoCodecTypeToString(VideoCodecType type)
    {
        switch (type)
        {
            case VideoCodecType.VP9:
                return "VP9";
            case VideoCodecType.VP8:
                return "VP8";
            case VideoCodecType.H264:
                return "H264";
            case VideoCodecType.AV1:
                return "AV1";
            case VideoCodecType.H265:
                return "H265";
            default:
                return "";
        }
    }
    static VideoCodecType VideoCodecTypeFromString(string value)
    {
        switch (value)
        {
            case "VP9":
                return VideoCodecType.VP9;
            case "VP8":
                return VideoCodecType.VP8;
            case "H264":
                return VideoCodecType.H264;
            case "AV1":
                return VideoCodecType.AV1;
            case "H265":
                return VideoCodecType.H265;
            default:
                return VideoCodecType.VP9;
        }
    }
    public enum AudioCodecType
    {
        OPUS,
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
    public enum DegradationPreference
    {
        // degradation preference を無効にします
        Disabled,
        // フレームレートを優先して維持し、帯域不足時は主に解像度を下げます
        MaintainFramerate,
        // 解像度を優先して維持し、帯域不足時は主にフレームレートを下げます
        MaintainResolution,
        // 解像度とフレームレートの両方をバランスよく下げます。
        Balanced,
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
        public List<string>? Header;
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
        public string? Name;
        public int? Priority;
        public class Rule
        {
            // 必須フィールドは空文字で初期化しておく
            public string Field = "";
            public string Operator = "";
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
    /// 各フィールドに直接値を入力しても構いませんが、FromDeviceCamera(), FromUnityCamera() または FromTexture() を使って生成することを推奨します。
    /// 
    /// Sora.GetVideoCapturerDevices() で取得した DeviceName または UniqueName を VideoCapturerDevice に設定することで、
    /// 指定したデバイスのカメラを利用することが出来ます。
    /// </remarks>
    public class CameraConfig
    {
        public CapturerType CapturerType = Sora.CapturerType.DeviceCamera;
        public UnityEngine.Camera? UnityCamera;
        public int UnityCameraRenderTargetDepthBuffer = 16;
        // 空文字ならデフォルトデバイスを利用する
        public string VideoCapturerDevice = "";
        public int VideoWidth = 640;
        public int VideoHeight = 480;
        public int VideoFps = 30;
        public UnityEngine.Texture? Texture;

        public static CameraConfig FromUnityCamera(UnityEngine.Camera unityCamera, int unityCameraRenderTargetDepthBuffer, int videoWidth, int videoHeight, int videoFps)
        {
            if (unityCamera == null)
            {
                throw new ArgumentNullException(nameof(unityCamera));
            }
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
        public static CameraConfig FromTexture(UnityEngine.Texture texture, int videoFps)
        {
            return new CameraConfig()
            {
                CapturerType = Sora.CapturerType.Texture,
                Texture = texture,
                VideoWidth = texture.width,
                VideoHeight = texture.height,
                VideoFps = videoFps,
            };
        }
    }

    public enum VideoCodecImplementation
    {
        // Internal は libwebrtc の内部で実装されているエンコーダー/デコーダーを利用する
        // 通常はソフトウェア実装だが、macOS, iOS, Android 等のプラットフォーム向けに一部ハードウェア実装も存在する
        Internal,
        CiscoOpenH264,
        IntelVpl,
        NvidiaVideoCodec,
        AmdAmf,
    }
    static string VideoCodecImplementationToString(VideoCodecImplementation implementation)
    {
        switch (implementation)
        {
            case VideoCodecImplementation.Internal:
                return "internal";
            case VideoCodecImplementation.CiscoOpenH264:
                return "cisco_openh264";
            case VideoCodecImplementation.IntelVpl:
                return "intel_vpl";
            case VideoCodecImplementation.NvidiaVideoCodec:
                return "nvidia_video_codec";
            case VideoCodecImplementation.AmdAmf:
                return "amd_amf";
            default:
                return "";
        }
    }
    static VideoCodecImplementation VideoCodecImplementationFromString(string value)
    {
        switch (value)
        {
            case "internal":
                return VideoCodecImplementation.Internal;
            case "cisco_openh264":
                return VideoCodecImplementation.CiscoOpenH264;
            case "intel_vpl":
                return VideoCodecImplementation.IntelVpl;
            case "nvidia_video_codec":
                return VideoCodecImplementation.NvidiaVideoCodec;
            case "amd_amf":
                return VideoCodecImplementation.AmdAmf;
            default:
                return VideoCodecImplementation.Internal;
        }
    }

    public struct VideoCodecCapabilityConfig
    {
        public string? OpenH264Path;
    }
    public class VideoCodecCapability
    {
        // Parameters 系は JSON 文字列で表現する
        // Unity に統一された汎用的な JSON ライブラリが無いので仕方がない

        public struct Codec
        {
            // このビデオコーデックの...
            public VideoCodecType Type;
            // ...エンコーダーが利用可能かどうか
            public bool Encoder;
            // ...デコーダーが利用可能かどうか
            public bool Decoder;
            // コーデック固有のパラメータ
            public string Parameters;
        }
        public struct Engine
        {
            public VideoCodecImplementation Name;
            public Codec[] Codecs;
            // 実装固有のパラメータ
            public string Parameters;
        }
        public Engine[] Engines = new Engine[0];

        public string ToJson()
        {
            var vcc = ConvertToInternalVideoCodecCapability(this);
            var size = sora_video_codec_capability_to_json_size(Jsonif.Json.ToJson(vcc));
            var buf = new byte[size];
            sora_video_codec_capability_to_json(Jsonif.Json.ToJson(vcc), buf, size);
            return System.Text.Encoding.UTF8.GetString(buf);
        }
    }

    public class VideoCodecPreference
    {
        public struct Codec
        {
            public VideoCodecType Type;
            public VideoCodecImplementation? Encoder;
            public VideoCodecImplementation? Decoder;
            // C++ SDK 的には必須なんだけど、struct では初期値を持てないので null 許容にしている
            // SoraConf.Internal 系の値に変換する時に null を "{}" に変えてやる
            public string? Parameters;
        }
        public Codec[] Codecs = new Codec[0];

        public string ToJson()
        {
            var vcp = ConvertToInternalVideoCodecPreference(this);
            var size = sora_video_codec_preference_to_json_size(Jsonif.Json.ToJson(vcp));
            var buf = new byte[size];
            sora_video_codec_preference_to_json(Jsonif.Json.ToJson(vcp), buf, size);
            return System.Text.Encoding.UTF8.GetString(buf);
        }
        // この VideoCodecPreference が、どれかのコーデックでこの implementation を使うように要求しているかどうか
        public bool HasImplementation(VideoCodecImplementation implementation)
        {
            var vcp = ConvertToInternalVideoCodecPreference(this);
            return sora_video_codec_preference_has_implementation(Jsonif.Json.ToJson(vcp), VideoCodecImplementationToString(implementation)) != 0;
        }
        // 自身のコーデックを、preference の同じコーデックで上書きする
        public void Merge(VideoCodecPreference preference)
        {
            var vcp = ConvertToInternalVideoCodecPreference(this);
            var vcp2 = ConvertToInternalVideoCodecPreference(preference);
            var size = sora_video_codec_preference_merge_size(Jsonif.Json.ToJson(vcp), Jsonif.Json.ToJson(vcp2));
            var buf = new byte[size];
            sora_video_codec_preference_merge(Jsonif.Json.ToJson(vcp), Jsonif.Json.ToJson(vcp2), buf, size);
            var vcpr = Jsonif.Json.FromJson<SoraConf.Internal.VideoCodecPreference>(System.Text.Encoding.UTF8.GetString(buf));
            var result = ConvertToVideoCodecPreference(vcpr);
            this.Codecs = result.Codecs;
        }
        // 指定した capability から implementation の実装だけを利用する VideoCodecPreference を生成する
        public static VideoCodecPreference CreateFromImplementation(VideoCodecCapability capability, VideoCodecImplementation implementation)
        {
            var vcc = ConvertToInternalVideoCodecCapability(capability);
            var size = sora_create_video_codec_preference_from_implementation_size(Jsonif.Json.ToJson(vcc), VideoCodecImplementationToString(implementation));
            var buf = new byte[size];
            sora_create_video_codec_preference_from_implementation(Jsonif.Json.ToJson(vcc), VideoCodecImplementationToString(implementation), buf, size);
            var vcpr = Jsonif.Json.FromJson<SoraConf.Internal.VideoCodecPreference>(System.Text.Encoding.UTF8.GetString(buf));
            return ConvertToVideoCodecPreference(vcpr);
        }

        // 可能な限り HWA を利用する VideoCodecPreference を返す
        // 優先度的には Intel VPL > AMD AMF > Nvidia Video Codec > Internal となる
        public static VideoCodecPreference GetHardwareAcceleratorPreference(VideoCodecCapability capability)
        {
            // Merge は同じコーデックを上書きするので、優先度が高いのを後でマージすることで正しい優先度になる
            var preference = new VideoCodecPreference();
            preference.Merge(CreateFromImplementation(capability, VideoCodecImplementation.Internal));
            preference.Merge(CreateFromImplementation(capability, VideoCodecImplementation.NvidiaVideoCodec));
            preference.Merge(CreateFromImplementation(capability, VideoCodecImplementation.AmdAmf));
            preference.Merge(CreateFromImplementation(capability, VideoCodecImplementation.IntelVpl));
            return preference;
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
        public VideoCodecType? VideoCodecType;
        public string VideoVp9Params = "";
        public string VideoAv1Params = "";
        public string VideoH264Params = "";
        public int VideoBitRate = 0;
        public DegradationPreference? DegradationPreference;
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
        public AudioCodecType? AudioCodecType;
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
        [System.Obsolete("forwardingFilter は非推奨です。代わりに forwardingFilters を使用してください。")]
        public ForwardingFilter? ForwardingFilter;
        public List<ForwardingFilter>? ForwardingFilters;

        // 利用するエンコーダー/デコーダーの指定
        // null の場合は VideoCodecImplementation.Internal な実装のみ利用する
        public VideoCodecPreference? VideoCodecPreference;

        // 証明書周りの設定
        public string? ClientCert;
        public string? ClientKey;
        public string? CACert;
    }

    IntPtr p;
    GCHandle selfHandle;
    Action<uint, string>? onAddTrack;
    Action<uint, string>? onRemoveTrack;
    Action<RtpTransceiver, MediaStreamTrack, string>? onMediaStreamTrack;
    Action<RtpReceiver, MediaStreamTrack, string>? onRemoveMediaStreamTrack;
    Action<string>? onSetOffer;
    Action<string>? onNotify;
    Action<string>? onPush;
    Action<string, byte[]>? onMessage;
    Action<SoraConf.ErrorCode, string>? onDisconnect;
    Action<string>? onDataChannel;
    Action<short[], int, int>? onHandleAudio;
    Action<SoraConf.VideoFrame>? onCapturerFrame;
    UnityEngine.Rendering.CommandBuffer commandBuffer;
    UnityEngine.Camera? unityCamera;

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

        foreach (var adapter in audioTrackSinks.Values)
        {
            adapter.Dispose();
        }
        audioTrackSinks.Clear();

        if (senderAudioSinkAdapter != null)
        {
            senderAudioSinkAdapter.Dispose();
            senderAudioSinkAdapter = null;
            senderAudioSink = null;
        }

        if (selfHandle.IsAllocated)
        {
            selfHandle.Free();
        }
    }

    public Sora()
    {
        p = sora_create();
        selfHandle = GCHandle.Alloc(this);
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
            if (config.CameraConfig.UnityCamera == null)
            {
                throw new ArgumentNullException(nameof(config.CameraConfig.UnityCamera), "CapturerType が UnityCamera の場合は UnityCamera を指定してください。");
            }
            unityCamera = config.CameraConfig.UnityCamera;
            var texture = new UnityEngine.RenderTexture(config.CameraConfig.VideoWidth, config.CameraConfig.VideoHeight, config.CameraConfig.UnityCameraRenderTargetDepthBuffer, UnityEngine.RenderTextureFormat.BGRA32);
            unityCamera.targetTexture = texture;
            unityCamera.enabled = true;
            unityCameraTexture = texture.GetNativeTexturePtr();
        }
        else if (config.CameraConfig.CapturerType == CapturerType.Texture)
        {
            if (config.CameraConfig.Texture == null)
            {
                throw new ArgumentNullException(nameof(config.CameraConfig.Texture), "CapturerType が Texture の場合は Texture を指定してください。");
            }
            unityCameraTexture = config.CameraConfig.Texture!.GetNativeTexturePtr();
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
        if (config.Multistream.HasValue)
        {
            cc.SetMultistream(config.Multistream.Value);
        }
        if (config.Spotlight.HasValue)
        {
            cc.SetSpotlight(config.Spotlight.Value);
        }
        cc.spotlight = config.Spotlight.GetValueOrDefault();
        cc.spotlight_number = config.SpotlightNumber;
        cc.spotlight_focus_rid = config.SpotlightFocusRid == null ? "" : config.SpotlightFocusRid.Value.ToString().ToLower();
        cc.spotlight_unfocus_rid = config.SpotlightUnfocusRid == null ? "" : config.SpotlightUnfocusRid.Value.ToString().ToLower();
        if (config.Simulcast.HasValue)
        {
            cc.SetSimulcast(config.Simulcast.Value);
        }
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
        cc.video_codec_type = config.VideoCodecType == null ? "" : config.VideoCodecType.ToString();
        cc.video_vp9_params = config.VideoVp9Params;
        cc.video_av1_params = config.VideoAv1Params;
        cc.video_h264_params = config.VideoH264Params;
        cc.video_bit_rate = config.VideoBitRate;
        if (config.DegradationPreference.HasValue)
        {
            switch (config.DegradationPreference.Value)
            {
                case DegradationPreference.Disabled:
                    cc.SetDegradationPreference(SoraConf.Internal.DegradationPreference.DISABLED);
                    break;
                case DegradationPreference.MaintainFramerate:
                    cc.SetDegradationPreference(SoraConf.Internal.DegradationPreference.MAINTAIN_FRAMERATE);
                    break;
                case DegradationPreference.MaintainResolution:
                    cc.SetDegradationPreference(SoraConf.Internal.DegradationPreference.MAINTAIN_RESOLUTION);
                    break;
                case DegradationPreference.Balanced:
                    cc.SetDegradationPreference(SoraConf.Internal.DegradationPreference.BALANCED);
                    break;
            }
        }
        cc.unity_audio_input = config.UnityAudioInput;
        cc.unity_audio_output = config.UnityAudioOutput;
        cc.audio_recording_device = config.AudioRecordingDevice;
        cc.audio_playout_device = config.AudioPlayoutDevice;
        cc.audio_codec_type = config.AudioCodecType == null ? "" : config.AudioCodecType.ToString();
        cc.audio_bit_rate = config.AudioBitRate;
        cc.audio_streaming_language_code = config.AudioStreamingLanguageCode;
        if (config.EnableDataChannelSignaling)
        {
            cc.SetDataChannelSignaling(config.DataChannelSignaling);
        }
        cc.data_channel_signaling_timeout = config.DataChannelSignalingTimeout;
        if (config.EnableIgnoreDisconnectWebsocket)
        {
            cc.SetIgnoreDisconnectWebsocket(config.IgnoreDisconnectWebsocket);
        }
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
                c.SetOrdered(m.Ordered.Value);
            }
            if (m.MaxPacketLifeTime != null)
            {
                c.SetMaxPacketLifeTime(m.MaxPacketLifeTime.Value);
            }
            if (m.MaxRetransmits != null)
            {
                c.SetMaxRetransmits(m.MaxRetransmits.Value);
            }
            if (m.Protocol != null)
            {
                c.SetProtocol(m.Protocol);
            }
            if (m.Compress != null)
            {
                c.SetCompress(m.Compress.Value);
            }
            if (m.Header != null)
            {
                c.SetHeader(new SoraConf.Internal.DataChannel.Header { content = m.Header });
            }
            cc.data_channels.Add(c);
        }
        cc.proxy_url = config.ProxyUrl;
        cc.proxy_username = config.ProxyUsername;
        cc.proxy_password = config.ProxyPassword;
        cc.proxy_agent = config.ProxyAgent;
        if (config.ForwardingFilter != null)
        {
            cc.SetForwardingFilter(ConvertToInternalForwardingFilter(config.ForwardingFilter));
        }

        if (config.ForwardingFilters != null)
        {
            var ffs = new SoraConf.Internal.ForwardingFilters();
            foreach (var filter in config.ForwardingFilters)
            {
                ffs.filters.Add(ConvertToInternalForwardingFilter(filter));
            }
            cc.SetForwardingFilters(ffs);
        }
        if (config.VideoCodecPreference != null)
        {
            cc.SetVideoCodecPreference(ConvertToInternalVideoCodecPreference(config.VideoCodecPreference));
        }
        if (config.ClientCert != null)
        {
            cc.SetClientCert(config.ClientCert);
        }
        if (config.ClientKey != null)
        {
            cc.SetClientKey(config.ClientKey);
        }
        if (config.CACert != null)
        {
            cc.SetCaCert(config.CACert);
        }

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

    static SoraConf.Internal.ForwardingFilter ConvertToInternalForwardingFilter(ForwardingFilter filter)
    {
        var ff = new SoraConf.Internal.ForwardingFilter();
        if (filter.Action != null)
        {
            ff.SetAction(filter.Action);
        }
        if (filter.Name != null)
        {
            ff.SetName(filter.Name);
        }
        if (filter.Priority.HasValue)
        {
            ff.SetPriority(filter.Priority.Value);
        }
        foreach (var rs in filter.Rules)
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
            ff.rules.Add(ccrs);
        }
        if (filter.Version != null)
        {
            ff.SetVersion(filter.Version);
        }
        if (filter.Metadata != null)
        {
            ff.SetMetadata(filter.Metadata);
        }
        return ff;
    }

    static SoraConf.Internal.VideoCodecCapabilityConfig ConvertToInternalVideoCodecCapabilityConfig(VideoCodecCapabilityConfig config)
    {
        var vccc = new SoraConf.Internal.VideoCodecCapabilityConfig();
        if (config.OpenH264Path != null)
        {
            vccc.openh264_path = config.OpenH264Path;
        }
        return vccc;
    }

    static VideoCodecCapability ConvertToVideoCodecCapability(SoraConf.Internal.VideoCodecCapability capability)
    {
        var vcc = new VideoCodecCapability();
        var vcces = new List<VideoCodecCapability.Engine>();
        foreach (var engine in capability.engines)
        {
            var e = new VideoCodecCapability.Engine();
            e.Name = VideoCodecImplementationFromString(engine.name);
            var ecs = new List<Sora.VideoCodecCapability.Codec>();
            foreach (var codec in engine.codecs)
            {
                var c = new VideoCodecCapability.Codec();
                c.Type = VideoCodecTypeFromString(codec.type);
                c.Encoder = codec.encoder;
                c.Decoder = codec.decoder;
                c.Parameters = codec.parameters;
                ecs.Add(c);
            }
            e.Codecs = ecs.ToArray();
            e.Parameters = engine.parameters;
            vcces.Add(e);
        }
        vcc.Engines = vcces.ToArray();
        return vcc;
    }
    static SoraConf.Internal.VideoCodecCapability ConvertToInternalVideoCodecCapability(VideoCodecCapability capability)
    {
        var vcc = new SoraConf.Internal.VideoCodecCapability();
        foreach (var engine in capability.Engines)
        {
            var e = new SoraConf.Internal.VideoCodecCapability.Engine();
            e.name = VideoCodecImplementationToString(engine.Name);
            foreach (var codec in engine.Codecs)
            {
                var c = new SoraConf.Internal.VideoCodecCapability.Codec();
                c.type = VideoCodecTypeToString(codec.Type);
                c.encoder = codec.Encoder;
                c.decoder = codec.Decoder;
                c.parameters = codec.Parameters;
                e.codecs.Add(c);
            }
            e.parameters = engine.Parameters;
            vcc.engines.Add(e);
        }
        return vcc;
    }

    static VideoCodecPreference ConvertToVideoCodecPreference(SoraConf.Internal.VideoCodecPreference preference)
    {
        var vcp = new VideoCodecPreference();
        var vcpcs = new List<VideoCodecPreference.Codec>();
        foreach (var codec in preference.codecs)
        {
            var c = new VideoCodecPreference.Codec();
            c.Type = VideoCodecTypeFromString(codec.type);
            if (codec.HasEncoder())
            {
                c.Encoder = VideoCodecImplementationFromString(codec.encoder);
            }
            if (codec.HasDecoder())
            {
                c.Decoder = VideoCodecImplementationFromString(codec.decoder);
            }
            c.Parameters = codec.parameters;
            vcpcs.Add(c);
        }
        vcp.Codecs = vcpcs.ToArray();
        return vcp;
    }
    static SoraConf.Internal.VideoCodecPreference ConvertToInternalVideoCodecPreference(VideoCodecPreference preference)
    {
        var vcp = new SoraConf.Internal.VideoCodecPreference();
        foreach (var codec in preference.Codecs)
        {
            var c = new SoraConf.Internal.VideoCodecPreference.Codec();
            c.type = VideoCodecTypeToString(codec.Type);
            if (codec.Encoder != null)
            {
                c.SetEncoder(VideoCodecImplementationToString(codec.Encoder.Value));
            }
            if (codec.Decoder != null)
            {
                c.SetDecoder(VideoCodecImplementationToString(codec.Decoder.Value));
            }
            c.parameters = codec.Parameters == null ? "{}" : codec.Parameters;
            vcp.codecs.Add(c);
        }
        return vcp;
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
            if (config.UnityCamera == null)
            {
                throw new ArgumentNullException(nameof(config.UnityCamera), "CapturerType が UnityCamera の場合は UnityCamera を指定してください。");
            }
            unityCamera = config.UnityCamera;
            var texture = new UnityEngine.RenderTexture(config.VideoWidth, config.VideoHeight, config.UnityCameraRenderTargetDepthBuffer, UnityEngine.RenderTextureFormat.BGRA32);
            unityCamera.targetTexture = texture;
            unityCamera.enabled = true;
            unityCameraTexture = texture.GetNativeTexturePtr();
        }
        else if (config.CapturerType == CapturerType.Texture)
        {
            if (config.Texture == null)
            {
                throw new ArgumentNullException(nameof(config.Texture), "CapturerType が Texture の場合は Texture を指定してください。");
            }
            unityCameraTexture = config.Texture!.GetNativeTexturePtr();
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
    /// videoSinkId から対応する VideoTrack を取得する
    /// </summary>
    public VideoTrack GetVideoTrackFromVideoSinkId(uint videoSinkId)
    {
        IntPtr trackPtr = sora_get_video_track_from_video_sink_id(p, videoSinkId);
        if (trackPtr == IntPtr.Zero)
        {
            throw new InvalidOperationException("指定した videoSinkId に対応する VideoTrack が見つかりません。");
        }
        return new VideoTrack(trackPtr);
    }

    /// <summary>
    /// videoSinkId で受信した映像を texture にレンダリングする
    /// </summary>
    /// <remarks>
    /// Role.Sendonly または Role.Sendrecv で映像を送信している場合、
    /// 自身の videoSinkId を指定すれば、自身が送信している映像を texture にレンダリングすることもできます。
    /// </remarks>
    public void RenderTrackToTexture(uint videoSinkId, UnityEngine.Texture texture)
    {
        commandBuffer.IssuePluginCustomTextureUpdateV2(sora_get_texture_update_callback(), texture, videoSinkId);
        UnityEngine.Graphics.ExecuteCommandBuffer(commandBuffer);
        commandBuffer.Clear();
    }

    private delegate void AddTrackCallbackDelegate(uint track_id, string connection_id, IntPtr userdata);

    [AOT.MonoPInvokeCallback(typeof(AddTrackCallbackDelegate))]
    static private void AddTrackCallback(uint videoSinkId, string connectionId, IntPtr userdata)
    {
        var sora = GCHandle.FromIntPtr(userdata).Target as Sora;
        sora!.onAddTrack!(videoSinkId, connectionId);
    }

    /// <summary>
    /// ビデオトラックが追加された時のコールバック
    /// </summary>
    /// <remarks>
    /// Action の引数は uint videoSinkId, string connectionId です。
    /// connectionId が空文字だった場合、送信者自身のトラックが追加されたことを表します。
    /// </remarks>
    public Action<uint, string>? OnAddTrack
    {
        set
        {
            onAddTrack = value;
            sora_set_on_add_track(p, value == null ? null : AddTrackCallback, GCHandle.ToIntPtr(selfHandle));
        }
    }

    private delegate void RemoveTrackCallbackDelegate(uint track_id, string connection_id, IntPtr userdata);

    [AOT.MonoPInvokeCallback(typeof(RemoveTrackCallbackDelegate))]
    static private void RemoveTrackCallback(uint videoSinkId, string connectionId, IntPtr userdata)
    {
        var sora = GCHandle.FromIntPtr(userdata).Target as Sora;
        sora!.onRemoveTrack!(videoSinkId, connectionId);
    }

    public Action<uint, string>? OnRemoveTrack
    {
        set
        {
            onRemoveTrack = value;
            sora_set_on_remove_track(p, value == null ? null : RemoveTrackCallback, GCHandle.ToIntPtr(selfHandle));
        }
    }

    private delegate void MediaStreamTrackCallbackDelegate(IntPtr transceiver, IntPtr track, string connectionId, IntPtr userdata);

    [AOT.MonoPInvokeCallback(typeof(MediaStreamTrackCallbackDelegate))]
    static private void MediaStreamTrackCallback(IntPtr transceiver, IntPtr track, string connectionId, IntPtr userdata)
    {
        var sora = GCHandle.FromIntPtr(userdata).Target as Sora;
        MediaStreamTrack managedTrack;
        var tmp = new MediaStreamTrack(track);
        if (tmp.Kind == MediaStreamTrack.VideoKind)
        {
            managedTrack = new VideoTrack(track);
        }
        else
        {
            managedTrack = new AudioTrack(track);
        }
        sora!.onMediaStreamTrack!(new RtpTransceiver(transceiver), managedTrack, connectionId);
    }

    /// <summary>
    /// トラックが追加された時のコールバック
    /// </summary>
    /// <remarks>
    /// Action の引数は RtpTransceiver transceiver, MediaStreamTrack track, string connectionId です。
    /// track は track.Kind の値によって AudioTrack または VideoTrack のいずれかになります。
    /// 
    /// OnAddTrack と違い、オーディオトラックでも呼び出されますが、
    /// 送信側のビデオ/オーディオが追加された場合には呼び出されません。
    /// （RTCPeerConnection の ontrack イベントに対応しています）
    /// </remarks>
    public Action<RtpTransceiver, MediaStreamTrack, string>? OnMediaStreamTrack
    {
        set
        {
            onMediaStreamTrack = value;
            sora_set_on_media_stream_track(p, value == null ? null : MediaStreamTrackCallback, GCHandle.ToIntPtr(selfHandle));
        }
    }

    private delegate void RemoveMediaStreamTrackCallbackDelegate(IntPtr receiver, IntPtr track, string connectionId, IntPtr userdata);

    [AOT.MonoPInvokeCallback(typeof(RemoveMediaStreamTrackCallbackDelegate))]
    static private void RemoveMediaStreamTrackCallback(IntPtr receiver, IntPtr track, string connectionId, IntPtr userdata)
    {
        var sora = GCHandle.FromIntPtr(userdata).Target as Sora;
        MediaStreamTrack managedTrack;
        var tmp = new MediaStreamTrack(track);
        if (tmp.Kind == MediaStreamTrack.VideoKind)
        {
            managedTrack = new VideoTrack(track);
        }
        else
        {
            managedTrack = new AudioTrack(track);
        }

        sora!.onRemoveMediaStreamTrack!(new RtpReceiver(receiver), managedTrack, connectionId);
    }

    /// <summary>
    /// トラックが削除された時のコールバック
    /// </summary>
    /// <remarks>
    /// Action の引数は RtpReceiver receiver, MediaStreamTrack track, string connectionId です。
    /// track は track.Kind の値によって AudioTrack または VideoTrack のいずれかになります。
    /// 
    /// OnRemoveTrack と違い、オーディオトラックでも呼び出されますが、
    /// 送信側のビデオ/オーディオが削除された場合には呼び出されません。
    /// </remarks>
    public Action<RtpReceiver, MediaStreamTrack, string>? OnRemoveMediaStreamTrack
    {
        set
        {
            onRemoveMediaStreamTrack = value;
            sora_set_on_remove_media_stream_track(p, value == null ? null : RemoveMediaStreamTrackCallback, GCHandle.ToIntPtr(selfHandle));
        }
    }

    private delegate void SetOfferCallbackDelegate(string json, IntPtr userdata);

    [AOT.MonoPInvokeCallback(typeof(SetOfferCallbackDelegate))]
    static private void SetOfferCallback(string json, IntPtr userdata)
    {
        var sora = GCHandle.FromIntPtr(userdata).Target as Sora;
        sora!.onSetOffer!(json);
    }

    public Action<string>? OnSetOffer
    {
        set
        {
            onSetOffer = value;
            sora_set_on_set_offer(p, value == null ? null : SetOfferCallback, GCHandle.ToIntPtr(selfHandle));
        }
    }

    private delegate void NotifyCallbackDelegate(string json, IntPtr userdata);

    [AOT.MonoPInvokeCallback(typeof(NotifyCallbackDelegate))]
    static private void NotifyCallback(string json, IntPtr userdata)
    {
        var sora = GCHandle.FromIntPtr(userdata).Target as Sora;
        sora!.onNotify!(json);
    }

    public Action<string>? OnNotify
    {
        set
        {
            onNotify = value;
            sora_set_on_notify(p, value == null ? null : NotifyCallback, GCHandle.ToIntPtr(selfHandle));
        }
    }

    private delegate void PushCallbackDelegate(string json, IntPtr userdata);

    [AOT.MonoPInvokeCallback(typeof(PushCallbackDelegate))]
    static private void PushCallback(string json, IntPtr userdata)
    {
        var sora = GCHandle.FromIntPtr(userdata).Target as Sora;
        sora!.onPush!(json);
    }

    public Action<string>? OnPush
    {
        set
        {
            onPush = value;
            sora_set_on_push(p, value == null ? null : PushCallback, GCHandle.ToIntPtr(selfHandle));
        }
    }

    private delegate void MessageCallbackDelegate(string label, IntPtr buf, int size, IntPtr userdata);

    [AOT.MonoPInvokeCallback(typeof(MessageCallbackDelegate))]
    static private void MessageCallback(string label, IntPtr buf, int size, IntPtr userdata)
    {
        var sora = GCHandle.FromIntPtr(userdata).Target as Sora;
        byte[] data = new byte[size];
        Marshal.Copy(buf, data, 0, size);
        sora!.onMessage!(label, data);
    }

    public Action<string, byte[]>? OnMessage
    {
        set
        {
            onMessage = value;
            sora_set_on_message(p, value == null ? null : MessageCallback, GCHandle.ToIntPtr(selfHandle));
        }
    }

    private delegate void DisconnectCallbackDelegate(int errorCode, string message, IntPtr userdata);

    [AOT.MonoPInvokeCallback(typeof(DisconnectCallbackDelegate))]
    static private void DisconnectCallback(int errorCode, string message, IntPtr userdata)
    {
        var sora = GCHandle.FromIntPtr(userdata).Target as Sora;
        sora!.onDisconnect!((SoraConf.ErrorCode)errorCode, message);
    }

    public Action<SoraConf.ErrorCode, string>? OnDisconnect
    {
        set
        {
            onDisconnect = value;
            sora_set_on_disconnect(p, value == null ? null : DisconnectCallback, GCHandle.ToIntPtr(selfHandle));
        }
    }

    private delegate void DataChannelCallbackDelegate(string label, IntPtr userdata);

    [AOT.MonoPInvokeCallback(typeof(DataChannelCallbackDelegate))]
    static private void DataChannelCallback(string label, IntPtr userdata)
    {
        var sora = GCHandle.FromIntPtr(userdata).Target as Sora;
        sora!.onDataChannel!(label);
    }

    public Action<string>? OnDataChannel
    {
        set
        {
            onDataChannel = value;
            sora_set_on_data_channel(p, value == null ? null : DataChannelCallback, GCHandle.ToIntPtr(selfHandle));
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
        var sora = GCHandle.FromIntPtr(userdata).Target as Sora;
        short[] buf2 = new short[samples * channels];
        Marshal.Copy(buf, buf2, 0, samples * channels);
        sora!.onHandleAudio!(buf2, samples, channels);
    }

    /// <summary>
    /// オーディオ再生用のコールバック
    /// </summary>
    /// <remarks>
    /// Config.UnityAudioOutput = true の場合のみ有効です。
    /// 再生デバイスで直接再生する代わりに、再生するためのデータをこのコールバックに渡します。
    /// また、このコールバックは Unity スレッドとは別のスレッドから呼ばれることに注意して下さい。
    /// </remarks>
    public Action<short[], int, int>? OnHandleAudio
    {
        set
        {
            onHandleAudio = value;
            sora_set_on_handle_audio(p, value == null ? null : HandleAudioCallback, GCHandle.ToIntPtr(selfHandle));
        }
    }

    private delegate void CapturerFrameCallbackDelegate(string data, IntPtr userdata);

    [AOT.MonoPInvokeCallback(typeof(CapturerFrameCallbackDelegate))]
    static private void CapturerFrameCallback(string data, IntPtr userdata)
    {
        var sora = GCHandle.FromIntPtr(userdata).Target as Sora;
        var frame = Jsonif.Json.FromJson<SoraConf.VideoFrame>(data);
        sora!.onCapturerFrame!(frame);
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
    public Action<SoraConf.VideoFrame>? OnCapturerFrame
    {
        set
        {
            onCapturerFrame = value;
            sora_set_on_capturer_frame(p, value == null ? null : CapturerFrameCallback, GCHandle.ToIntPtr(selfHandle));
        }
    }

    private delegate void StatsCallbackDelegate(string json, IntPtr userdata);

    [AOT.MonoPInvokeCallback(typeof(StatsCallbackDelegate))]
    static private void StatsCallback(string json, IntPtr userdata)
    {
        GCHandle handle = GCHandle.FromIntPtr(userdata);
        var callback = GCHandle.FromIntPtr(userdata).Target as Action<string>;
        callback!(json);
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
        callback!(device_name, unique_name);
    }

    public struct DeviceInfo
    {
        public string DeviceName;
        public string UniqueName;
    }
    public static DeviceInfo[]? GetVideoCapturerDevices()
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

    public static DeviceInfo[]? GetAudioRecordingDevices()
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

    public static DeviceInfo[]? GetAudioPlayoutDevices()
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

    public static VideoCodecCapability GetVideoCodecCapability(VideoCodecCapabilityConfig config)
    {
        var c = ConvertToInternalVideoCodecCapabilityConfig(config);
        var cjson = Jsonif.Json.ToJson(c);
        int size = sora_get_video_codec_capability_size(cjson);
        byte[] buf = new byte[size];
        sora_get_video_codec_capability(cjson, buf, size);
        var vcc = Jsonif.Json.FromJson<SoraConf.Internal.VideoCodecCapability>(System.Text.Encoding.UTF8.GetString(buf));
        return ConvertToVideoCodecCapability(vcc);
    }

#if UNITY_IOS && !UNITY_EDITOR
    private const string DllName = "__Internal";
#else
    private const string DllName = "SoraUnitySdk";
#endif

    [DllImport(DllName)]
    private static extern IntPtr sora_create();
    [DllImport(DllName)]
    private static extern void sora_set_on_add_track(IntPtr p, AddTrackCallbackDelegate? on_add_track, IntPtr userdata);
    [DllImport(DllName)]
    private static extern void sora_set_on_remove_track(IntPtr p, RemoveTrackCallbackDelegate? on_remove_track, IntPtr userdata);
    [DllImport(DllName)]
    private static extern void sora_set_on_media_stream_track(IntPtr p, MediaStreamTrackCallbackDelegate? on_media_stream_track, IntPtr userdata);
    [DllImport(DllName)]
    private static extern void sora_set_on_remove_media_stream_track(IntPtr p, RemoveMediaStreamTrackCallbackDelegate? on_remove_media_stream_track, IntPtr userdata);
    [DllImport(DllName)]
    private static extern void sora_set_on_set_offer(IntPtr p, SetOfferCallbackDelegate? on_set_offer, IntPtr userdata);
    [DllImport(DllName)]
    private static extern void sora_set_on_notify(IntPtr p, NotifyCallbackDelegate? on_notify, IntPtr userdata);
    [DllImport(DllName)]
    private static extern void sora_set_on_push(IntPtr p, PushCallbackDelegate? on_push, IntPtr userdata);
    [DllImport(DllName)]
    private static extern void sora_set_on_message(IntPtr p, MessageCallbackDelegate? on_message, IntPtr userdata);
    [DllImport(DllName)]
    private static extern void sora_set_on_disconnect(IntPtr p, DisconnectCallbackDelegate? on_disconnect, IntPtr userdata);
    [DllImport(DllName)]
    private static extern void sora_set_on_data_channel(IntPtr p, DataChannelCallbackDelegate? on_data_channel, IntPtr userdata);
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
    private static extern void sora_set_on_handle_audio(IntPtr p, HandleAudioCallbackDelegate? on_handle_audio, IntPtr userdata);
    [DllImport(DllName)]
    private static extern void sora_set_sender_audio_sink(IntPtr p, IntPtr sink);
    [DllImport(DllName)]
    private static extern void sora_set_on_capturer_frame(IntPtr p, CapturerFrameCallbackDelegate? on_capturer_frame, IntPtr userdata);
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
    [DllImport(DllName)]
    private static extern int sora_get_video_codec_capability_size(string config);
    [DllImport(DllName)]
    private static extern void sora_get_video_codec_capability(string config, [Out] byte[] buf, int size);
    [DllImport(DllName)]
    private static extern int sora_video_codec_preference_has_implementation(string self, string implementation);
    [DllImport(DllName)]
    private static extern int sora_video_codec_preference_merge_size(string self, string preference);
    [DllImport(DllName)]
    private static extern void sora_video_codec_preference_merge(string self, string preference, [Out] byte[] buf, int size);
    [DllImport(DllName)]
    private static extern int sora_create_video_codec_preference_from_implementation_size(string capability, string implementation);
    [DllImport(DllName)]
    private static extern void sora_create_video_codec_preference_from_implementation(string capability, string implementation, [Out] byte[] buf, int size);
    [DllImport(DllName)]
    private static extern int sora_video_codec_capability_to_json_size(string self);
    [DllImport(DllName)]
    private static extern void sora_video_codec_capability_to_json(string self, [Out] byte[] buf, int size);
    [DllImport(DllName)]
    private static extern int sora_video_codec_preference_to_json_size(string self);
    [DllImport(DllName)]
    private static extern void sora_video_codec_preference_to_json(string self, [Out] byte[] buf, int size);
    [DllImport(DllName)]
    private static extern int sora_media_stream_track_get_kind_size(IntPtr p);
    [DllImport(DllName)]
    private static extern void sora_media_stream_track_get_kind(IntPtr p, [Out] byte[] buf, int size);
    [DllImport(DllName)]
    private static extern int sora_media_stream_track_get_id_size(IntPtr p);
    [DllImport(DllName)]
    private static extern void sora_media_stream_track_get_id(IntPtr p, [Out] byte[] buf, int size);

    private delegate void sora_audio_track_on_data_fn(IntPtr buf, int bits_per_sample, int sample_rate, int number_of_channels, int number_of_frames, IntPtr absolute_capture_timestamp_ms, IntPtr userdata);
    private delegate int sora_audio_track_num_preferred_channels_fn(IntPtr userdata);
    [DllImport(DllName)]
    private static extern IntPtr sora_audio_track_sink_create(sora_audio_track_on_data_fn on_data, sora_audio_track_num_preferred_channels_fn num_preferred_channels, IntPtr userdata);
    [DllImport(DllName)]
    private static extern void sora_audio_track_sink_destroy(IntPtr p);
    [DllImport(DllName)]
    private static extern void sora_audio_track_add_sink(IntPtr track, IntPtr sink);
    [DllImport(DllName)]
    private static extern void sora_audio_track_remove_sink(IntPtr track, IntPtr sink);

    [DllImport(DllName)]
    private static extern IntPtr sora_get_video_track_from_video_sink_id(IntPtr p, uint videoSinkId);
    [DllImport(DllName)]
    private static extern uint sora_get_video_sink_id_from_video_track(IntPtr p, IntPtr videoTrack);

    [DllImport(DllName)]
    private static extern IntPtr sora_rtp_transceiver_get_receiver(IntPtr p);

    [DllImport(DllName)]
    private static extern int sora_rtp_receiver_get_info_size(IntPtr p);
    [DllImport(DllName)]
    private static extern void sora_rtp_receiver_get_info(IntPtr p, [Out] byte[] buf, int size);

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
        private bool disposed = false;

        private class ChangeRouteCallbackProxy : AndroidJavaProxy
        {
            public Action? onChangeRoute;

            public ChangeRouteCallbackProxy() : base("jp.shiguredo.sora.audiomanager.SoraAudioManager$OnChangeRouteObserver")
            {
            }

            public void OnChangeRoute()
            {
                onChangeRoute?.Invoke();
            }
        }

        public AndroidAudioOutputHelper(Action? onChangeRoute)
        {
            using (var soraAudioManagerFactoryClass = new AndroidJavaClass("jp.shiguredo.sora.audiomanager.SoraAudioManagerFactory"))
            {
                AndroidJavaClass unityPlayerClass = new AndroidJavaClass("com.unity3d.player.UnityPlayer");
                AndroidJavaObject currentActivity = unityPlayerClass.GetStatic<AndroidJavaObject>("currentActivity");
                soraAudioManager = soraAudioManagerFactoryClass.CallStatic<AndroidJavaObject>("createWithMainThreadWrapper", currentActivity);

                callbackProxy = new ChangeRouteCallbackProxy();
                if (onChangeRoute != null)
                {
                    callbackProxy.onChangeRoute += onChangeRoute;
                }
                soraAudioManager.Call("start", callbackProxy);
            }
        }

        public void Dispose()
        {
            if (disposed)
            {
                return;
            }
            disposed = true;

            soraAudioManager.Call("stop");
            soraAudioManager.Dispose();
        }

        public bool IsHandsfree()
        {
            if (disposed)
            {
                throw new ObjectDisposedException(nameof(AndroidAudioOutputHelper));
            }
            return soraAudioManager.Call<bool>("isHandsfree");
        }

        public void SetHandsfree(bool enabled)
        {
            if (disposed)
            {
                throw new ObjectDisposedException(nameof(AndroidAudioOutputHelper));
            }
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
            callback!();
        }

        GCHandle onChangeRouteHandle;
        IntPtr p;

        public DefaultAudioOutputHelper(Action? onChangeRoute)
        {
            if (onChangeRoute == null)
            {
                onChangeRoute = () => { }; // 空のアクションを割り当てる
            }

            onChangeRouteHandle = GCHandle.Alloc(onChangeRoute);
            p = sora_audio_output_helper_create(ChangeRouteCallback, GCHandle.ToIntPtr(onChangeRouteHandle));
        }

        public void Dispose()
        {
            if (p != IntPtr.Zero)
            {
                // ネイティブ側がコールバックを保持している可能性があるため、
                // 先にネイティブリソースを破棄してから GCHandle を解放する
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
        public static IAudioOutputHelper Create(Action? onChangeRoute)
        {
#if UNITY_ANDROID && !UNITY_EDITOR
            return new AndroidAudioOutputHelper(onChangeRoute);
#else
            return new DefaultAudioOutputHelper(onChangeRoute);
#endif
        }
    }

    /// <summary>
    /// メディアストリームトラックを表すクラス
    /// </summary>
    /// <remarks>
    /// webrtc::MediaStreamTrackInterface に対応しています。
    /// 
    /// 注意：このクラスおよび派生クラスはネイティブオブジェクトを所有しません。
    /// C++ 側でネイティブオブジェクトが解放されると、
    /// このクラスに対するあらゆる操作が未定義動作になります。
    /// </remarks>
    public class MediaStreamTrack
    {
        internal IntPtr p;

        internal MediaStreamTrack(IntPtr p)
        {
            this.p = p;
        }

        public const string AudioKind = "audio";
        public const string VideoKind = "video";

        public string Kind
        {
            get
            {
                int size = sora_media_stream_track_get_kind_size(p);
                byte[] buf = new byte[size];
                sora_media_stream_track_get_kind(p, buf, size);
                return System.Text.Encoding.UTF8.GetString(buf);
            }
        }

        public string Id
        {
            get
            {
                int size = sora_media_stream_track_get_id_size(p);
                byte[] buf = new byte[size];
                sora_media_stream_track_get_id(p, buf, size);
                return System.Text.Encoding.UTF8.GetString(buf);
            }
        }
    }

    /// <summary>
    /// ビデオトラックを表すクラス
    /// </summary>
    /// <remarks>
    /// webrtc::VideoTrackInterface に対応しています。
    /// </remarks>
    public class VideoTrack : MediaStreamTrack
    {
        internal VideoTrack(IntPtr p) : base(p)
        {
        }

        public uint GetVideoSinkId(Sora sora)
        {
            return sora_get_video_sink_id_from_video_track(sora.p, this.p);
        }

        // TODO(melpon): 必要になったら実装する
        // void AddOrUpdateSink(IVideoSink sink)
        // {
        // }
        // void RemoveSink(IVideoSink sink)
        // {
        // }
    }

    public interface IAudioTrackSink
    {
        void OnData(short[] data, int bitsPerSample, int sampleRate, int numberOfChannels, int numberOfFrames, long? absoluteCaptureTimestampMs);
    }

    internal class AudioTrackSinkAdapter : IDisposable
    {
        private IAudioTrackSink sink;
        private GCHandle selfHandle;
        private sora_audio_track_on_data_fn onDataThunk;
        private sora_audio_track_num_preferred_channels_fn numPreferredChannelsThunk;
        private IntPtr p;

        internal AudioTrackSinkAdapter(IAudioTrackSink sink)
        {
            this.sink = sink;
            selfHandle = GCHandle.Alloc(this);
            onDataThunk = OnDataThunk;
            numPreferredChannelsThunk = NumPreferredChannelsThunk;
            p = sora_audio_track_sink_create(onDataThunk, numPreferredChannelsThunk, GCHandle.ToIntPtr(selfHandle));
        }

        public void Dispose()
        {
            if (p != IntPtr.Zero)
            {
                sora_audio_track_sink_destroy(p);
                p = IntPtr.Zero;
            }
            if (selfHandle.IsAllocated)
            {
                selfHandle.Free();
            }
        }

        internal IntPtr Ptr
        {
            get { return p; }
        }

        [AOT.MonoPInvokeCallback(typeof(sora_audio_track_on_data_fn))]
        private static void OnDataThunk(IntPtr buf, int bitsPerSample, int sampleRate, int numberOfChannels, int numberOfFrames, IntPtr absoluteCaptureTimestampMs, IntPtr userdata)
        {
            var adapter = GCHandle.FromIntPtr(userdata).Target as AudioTrackSinkAdapter;
            short[] data = new short[numberOfChannels * numberOfFrames];
            Marshal.Copy(buf, data, 0, numberOfChannels * numberOfFrames);
            long? ts = absoluteCaptureTimestampMs == IntPtr.Zero ? (long?)null : Marshal.ReadInt64(absoluteCaptureTimestampMs);
            adapter!.sink.OnData(data, bitsPerSample, sampleRate, numberOfChannels, numberOfFrames, ts);
        }

        [AOT.MonoPInvokeCallback(typeof(sora_audio_track_num_preferred_channels_fn))]
        private static int NumPreferredChannelsThunk(IntPtr userdata)
        {
            return -1;
        }
    }

    Dictionary<IAudioTrackSink, AudioTrackSinkAdapter> audioTrackSinks = new Dictionary<IAudioTrackSink, AudioTrackSinkAdapter>();
    IAudioTrackSink? senderAudioSink = null;
    AudioTrackSinkAdapter? senderAudioSinkAdapter = null;

    public class AudioTrack : MediaStreamTrack
    {
        internal AudioTrack(IntPtr p) : base(p)
        {
        }

        public void AddSink(Sora sora, IAudioTrackSink sink)
        {
            if (sora.audioTrackSinks.ContainsKey(sink))
            {
                // すでに追加されている
                throw new InvalidOperationException("The sink has already been added.");
            }
            var adapter = new AudioTrackSinkAdapter(sink);
            sora_audio_track_add_sink(this.p, adapter.Ptr);
            sora.audioTrackSinks[sink] = adapter;
        }

        public void RemoveSink(Sora sora, IAudioTrackSink sink)
        {
            if (!sora.audioTrackSinks.ContainsKey(sink))
            {
                return;
            }
            var adapter = sora.audioTrackSinks[sink];
            sora_audio_track_remove_sink(this.p, adapter.Ptr);
            adapter.Dispose();
            sora.audioTrackSinks.Remove(sink);
        }
    }

    public IAudioTrackSink? SenderAudioSink
    {
        get { return senderAudioSink; }
        set
        {
            if (value == null)
            {
                if (this.senderAudioSinkAdapter != null)
                {
                    sora_set_sender_audio_sink(this.p, IntPtr.Zero);
                    this.senderAudioSinkAdapter.Dispose();
                    this.senderAudioSinkAdapter = null;
                    this.senderAudioSink = null;
                }
                return;
            }
            var adapter = new AudioTrackSinkAdapter(value);
            sora_set_sender_audio_sink(this.p, adapter.Ptr);
            this.senderAudioSink = value;
            this.senderAudioSinkAdapter = adapter;
        }
    }

    /// <summary>
    /// トランシーバを表すクラス
    /// </summary>
    /// <remarks>
    /// webrtc::RtpTransceiver に対応しています。
    /// 
    /// 注意：このクラスはネイティブオブジェクトを所有しません。
    /// C++ 側でネイティブオブジェクトが解放されると、
    /// このクラスに対するあらゆる操作が未定義動作になります。
    /// </remarks>
    public class RtpTransceiver
    {
        private IntPtr p;

        internal RtpTransceiver(IntPtr p)
        {
            this.p = p;
        }

        public RtpReceiver Receiver
        {
            get
            {
                return new RtpReceiver(sora_rtp_transceiver_get_receiver(p));
            }
        }
    }

    /// <summary>
    /// レシーバーを表すクラス
    /// </summary>
    /// <remarks>
    /// webrtc::RtpReceiver に対応しています。
    /// 
    /// 注意：このクラスはネイティブオブジェクトを所有しません。
    /// C++ 側でネイティブオブジェクトが解放されると、
    /// このクラスに対するあらゆる操作が未定義動作になります。
    /// </remarks>
    public class RtpReceiver
    {
        private IntPtr p;

        internal RtpReceiver(IntPtr p)
        {
            this.p = p;
        }

        internal SoraConf.Internal.RtpReceiverInfo GetInfo()
        {
            int size = sora_rtp_receiver_get_info_size(p);
            byte[] buf = new byte[size];
            sora_rtp_receiver_get_info(p, buf, size);
            var info = Jsonif.Json.FromJson<SoraConf.Internal.RtpReceiverInfo>(System.Text.Encoding.UTF8.GetString(buf));
            return info;
        }

        public string[] StreamIds
        {
            get
            {
                var info = GetInfo();
                return info.stream_ids.ToArray();
            }
        }

        public string Id
        {
            get
            {
                var info = GetInfo();
                return info.id;
            }
        }
    }
}
