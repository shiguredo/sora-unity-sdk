cmake_minimum_required(VERSION 3.16)

# Only interpret if() arguments as variables or keywords when unquoted.
cmake_policy(SET CMP0054 NEW)
# MSVC runtime library flags are selected by an abstraction.
cmake_policy(SET CMP0091 NEW)

if (SORA_UNITY_SDK_PACKAGE STREQUAL "android")
  # toolchain の設定
  set(CMAKE_TOOLCHAIN_FILE ${ANDROID_TOOLCHAIN_FILE})
  set(ANDROID_NATIVE_API_LEVEL 26)
  set(ANDROID_ABI arm64-v8a)
  set(ANDROID_STL none)
endif()

project(SoraUnitySdk C CXX)

set(Boost_USE_STATIC_LIBS ON)

set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)

find_package(WebRTC REQUIRED)
find_package(Boost REQUIRED)
find_package(JSON REQUIRED)

set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

if (SORA_UNITY_SDK_PACKAGE STREQUAL "windows")
  add_library(SoraUnitySdk SHARED)

  # configure_file で使う
  set(SORA_UNITY_SDK_PLATFORM Windows)
elseif (SORA_UNITY_SDK_PACKAGE STREQUAL "macos")
  add_library(SoraUnitySdk MODULE)
  set_target_properties(SoraUnitySdk PROPERTIES BUNDLE TRUE)
  set_target_properties(SoraUnitySdk PROPERTIES CXX_VISIBILITY_PRESET hidden)
  target_link_options(SoraUnitySdk PRIVATE -ObjC)

  # configure_file で使う
  set(SORA_UNITY_SDK_PLATFORM macOS)
elseif (SORA_UNITY_SDK_PACKAGE STREQUAL "android")
  add_library(SoraUnitySdk SHARED)

  set(SORA_UNITY_SDK_PLATFORM Android)
endif()

set_target_properties(SoraUnitySdk PROPERTIES CXX_STANDARD 14 C_STANDARD 99)

target_sources(SoraUnitySdk
  PRIVATE
    src/sora_signaling.cpp
    src/unity.cpp
    src/sora.cpp
    src/id_pointer.cpp
    src/unity_context.cpp
    src/unity_renderer.cpp
    src/unity_camera_capturer.cpp
    src/rtc/device_list.cpp
    src/rtc/device_video_capturer.cpp
    src/rtc/native_buffer.cpp
    src/rtc/observer.cpp
    src/rtc/rtc_connection.cpp
    src/rtc/rtc_manager.cpp
    src/rtc/scalable_track_source.cpp
    src/rtc/h264_format.cpp
)

string(SUBSTRING ${SORA_UNITY_SDK_COMMIT} 0 8 SORA_UNITY_SDK_COMMIT_SHORT)
string(SUBSTRING ${WEBRTC_SRC_COMMIT} 0 8 WEBRTC_SRC_COMMIT_SHORT)

configure_file(src/sora_version.h.template ${CMAKE_CURRENT_BINARY_DIR}/sora_version.h)

target_include_directories(SoraUnitySdk
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/NvCodec/include
    ${CMAKE_CURRENT_SOURCE_DIR}/NvCodec/NvCodec
)

target_link_libraries(SoraUnitySdk
  PRIVATE
    WebRTC::WebRTC
    Boost::boost
    JSON::JSON
)

if (SORA_UNITY_SDK_PACKAGE STREQUAL "windows")
  # 文字コードを utf-8 として扱うのと、シンボルテーブル数を増やす
  target_compile_options(SoraUnitySdk
    PRIVATE
      "$<$<COMPILE_LANGUAGE:CXX>:/utf-8;/bigobj>"
      "$<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler;/utf-8 /bigobj>"
  )
  set_target_properties(SoraUnitySdk
    PROPERTIES
      # CRTライブラリを静的リンクさせる
      # CMake 3.15 以上なら MSVC_RUNTIME_LIBRARY が使える
      MSVC_RUNTIME_LIBRARY "MultiThreaded"
  )

  target_sources(SoraUnitySdk
    PRIVATE
      src/unity_camera_capturer_d3d11.cpp
      src/rtc/hw_video_encoder_factory.cpp
      src/rtc/hw_video_decoder_factory.cpp
      src/hwenc_nvcodec/nvcodec_h264_encoder.cpp
      src/hwenc_nvcodec/nvcodec_video_decoder.cpp
      NvCodec/NvCodec/NvEncoder/NvEncoder.cpp
      NvCodec/NvCodec/NvEncoder/NvEncoderD3D11.cpp
  )

  target_link_libraries(SoraUnitySdk
    PRIVATE
      dbghelp.lib
      delayimp.lib
      dnsapi.lib
      msimg32.lib
      oleaut32.lib
      psapi.lib
      shell32.lib
      shlwapi.lib
      usp10.lib
      version.lib
      wininet.lib
      winmm.lib
      ws2_32.lib
      amstrmid.lib
      Strmiids.lib
      crypt32.lib
      dmoguids.lib
      iphlpapi.lib
      msdmo.lib
      Secur32.lib
      wmcodecdspuuid.lib
      dxgi.lib
      D3D11.lib
  )

  target_compile_definitions(SoraUnitySdk
    PRIVATE
      SORA_UNITY_SDK_WINDOWS
      UNICODE
      _UNICODE
      _CONSOLE
      _WIN32_WINNT=0x0A00
      WEBRTC_WIN
      NOMINMAX
      WIN32_LEAN_AND_MEAN
  )

  # enable_language(CUDA) は CUDA 用 Visual Studio Integration を利用して CUDA を検出しているが、
  # CUDA ドライバがインストールできない場所では VS Integration をインストールできないため、
  # enable_language(CUDA) は利用できない。
  # なので（deprecated だけど）FindCUDA を利用してコンパイルする。

  set(CUDA_TOOLKIT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/_install/cuda/nvcc)
  find_package(CUDA REQUIRED)

  set_source_files_properties(
      NvCodec/NvCodec/NvDecoder/NvDecoder.cpp
      src/hwenc_nvcodec/nvcodec_video_decoder_cuda.cpp
    PROPERTIES
      CUDA_SOURCE_PROPERTY_FORMAT OBJ
  )
  cuda_compile(CUDA_FILES
      NvCodec/NvCodec/NvDecoder/NvDecoder.cpp
      src/hwenc_nvcodec/nvcodec_video_decoder_cuda.cpp
    OPTIONS
      -Xcompiler /utf-8
      -Xcompiler /I${CMAKE_CURRENT_SOURCE_DIR}/NvCodec/include
      -Xcompiler /I${CMAKE_CURRENT_SOURCE_DIR}/NvCodec/NvCodec
  )
  target_sources(SoraUnitySdk PRIVATE ${CUDA_FILES})
  target_include_directories(SoraUnitySdk PRIVATE ${CUDA_INCLUDE_DIRS})
  target_link_libraries(SoraUnitySdk
    PRIVATE
      ${CUDA_TOOLKIT_ROOT_DIR}/lib/x64/cuda.lib
      ${CUDA_LIBRARIES}
      ${CMAKE_CURRENT_SOURCE_DIR}/NvCodec/Lib/x64/nvcuvid.lib
  )

  # CUDA 
  # enable_language(CUDA)
  # target_sources(SoraUnitySdk
  #   PRIVATE
  #     NvCodec/NvCodec/NvDecoder/NvDecoder.cpp
  #     src/hwenc_nvcodec/nvcodec_video_decoder_cuda.cpp
  # )
  # set_source_files_properties(
  #     NvCodec/NvCodec/NvDecoder/NvDecoder.cpp
  #     src/hwenc_nvcodec/nvcodec_video_decoder_cuda.cpp
  #   PROPERTIES
  #     LANGUAGE CUDA
  #     CUDA_SEPARABLE_COMPILATION ON
  # )
  # target_link_libraries(SoraUnitySdk
  #   PRIVATE
  #     cuda.lib
  #     ${CMAKE_CURRENT_SOURCE_DIR}/NvCodec/Lib/x64/nvcuvid.lib
  # )
elseif (SORA_UNITY_SDK_PACKAGE STREQUAL "macos")
  enable_language(OBJCXX)

  target_sources(SoraUnitySdk
    PRIVATE
      src/unity_camera_capturer_metal.mm
      src/mac_helper/mac_capturer.mm
      src/mac_helper/objc_codec_factory_helper.mm
  )

  target_link_libraries(SoraUnitySdk
    PRIVATE
      "-framework Foundation"
      "-framework AVFoundation"
      "-framework CoreServices"
      "-framework CoreFoundation"
      "-framework AudioUnit"
      "-framework AudioToolbox"
      "-framework CoreAudio"
      "-framework CoreGraphics"
      "-framework CoreMedia"
      "-framework CoreVideo"
      "-framework VideoToolbox"
      "-framework AppKit"
      "-framework Metal"
      "-framework MetalKit"
      "-framework OpenGL"
  )

  target_compile_definitions(SoraUnitySdk
    PRIVATE
      SORA_UNITY_SDK_MACOS
      WEBRTC_POSIX
      WEBRTC_MAC
  )

elseif (SORA_UNITY_SDK_PACKAGE STREQUAL "android")

  set(_INSTALL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/_install)

  find_library(ANDROID_LIB_LOG log)
  find_library(ANDROID_LIB_ANDROID android)
  find_library(ANDROID_LIB_OPENSLES OpenSLES)
  find_library(ANDROID_LIB_VULKAN vulkan)

  target_sources(SoraUnitySdk
    PRIVATE
      src/jni_onload.cc
      src/rtc/hw_video_encoder_factory.cpp
      src/rtc/hw_video_decoder_factory.cpp
      src/android_helper/android_codec_factory_helper.cpp
      src/android_helper/android_capturer.cpp
      src/android_helper/android_context.cpp
      src/unity_camera_capturer_vulkan.cpp
  )

  target_compile_definitions(SoraUnitySdk
    PRIVATE
      SORA_UNITY_SDK_ANDROID
      WEBRTC_POSIX
      WEBRTC_LINUX
      WEBRTC_ANDROID
      _LIBCPP_ABI_UNSTABLE
      SORA_UNITY_SDK_ANDROID
      _LIBCPP_DISABLE_VISIBILITY_ANNOTATIONS
      _LIBCXXABI_DISABLE_VISIBILITY_ANNOTATIONS
      _LIBCPP_ENABLE_NODISCARD
  )
  target_compile_options(SoraUnitySdk PRIVATE "-nostdinc++")
  target_include_directories(SoraUnitySdk
    PRIVATE
      ${_INSTALL_DIR}/libcxx/include
      ${_INSTALL_DIR}/libcxxabi/include
  )

  target_link_libraries(SoraUnitySdk
    PRIVATE
      ${ANDROID_LIB_LOG}
      ${ANDROID_LIB_ANDROID}
      ${ANDROID_LIB_OPENSLES}
      ${ANDROID_LIB_VULKAN}
  )
  target_link_options(SoraUnitySdk
    PRIVATE
      -fuse-ld=lld
      -Wl,--wrap=malloc
      -Wl,--wrap=calloc
      -Wl,--wrap=memalign
      -Wl,--wrap=realloc
      -Wl,--wrap=free
      # ./aarch64-linux-android-readelf -Ws ../../../../../../../_install/android/webrtc/lib/arm64-v8a/libwebrtc.a | grep Java_org_webrtc_ | while read a b c d e f g h; do echo -Wl,--undefined=$h; done
      -Wl,--undefined=Java_org_webrtc_Metrics_nativeEnable
      -Wl,--undefined=Java_org_webrtc_Metrics_nativeGetAndReset
      -Wl,--undefined=Java_org_webrtc_VideoDecoderFallback_nativeCreateDecoder
      -Wl,--undefined=Java_org_webrtc_NV21Buffer_nativeCropAndScale
      -Wl,--undefined=Java_org_webrtc_VideoEncoderFallback_nativeCreateEncoder
      -Wl,--undefined=Java_org_webrtc_JavaI420Buffer_nativeCropAndScaleI420
      -Wl,--undefined=Java_org_webrtc_VideoTrack_nativeAddSink
      -Wl,--undefined=Java_org_webrtc_VideoTrack_nativeFreeSink
      -Wl,--undefined=Java_org_webrtc_VideoTrack_nativeRemoveSink
      -Wl,--undefined=Java_org_webrtc_VideoTrack_nativeWrapSink
      -Wl,--undefined=Java_org_webrtc_VideoEncoderWrapper_nativeOnEncodedFrame
      -Wl,--undefined=Java_org_webrtc_VideoDecoderWrapper_nativeOnDecodedFrame
      -Wl,--undefined=Java_org_webrtc_NativeAndroidVideoTrackSource_nativeAdaptFrame
      -Wl,--undefined=Java_org_webrtc_NativeAndroidVideoTrackSource_nativeAdaptOutputFormat
      -Wl,--undefined=Java_org_webrtc_NativeAndroidVideoTrackSource_nativeOnFrameCaptured
      -Wl,--undefined=Java_org_webrtc_NativeAndroidVideoTrackSource_nativeSetIsScreencast
      -Wl,--undefined=Java_org_webrtc_NativeAndroidVideoTrackSource_nativeSetState
      -Wl,--undefined=Java_org_webrtc_NV12Buffer_nativeCropAndScale
      -Wl,--undefined=Java_org_webrtc_H264Utils_nativeIsSameH264Profile
      -Wl,--undefined=Java_org_webrtc_TimestampAligner_nativeCreateTimestampAligner
      -Wl,--undefined=Java_org_webrtc_TimestampAligner_nativeReleaseTimestampAligner
      -Wl,--undefined=Java_org_webrtc_TimestampAligner_nativeRtcTimeNanos
      -Wl,--undefined=Java_org_webrtc_TimestampAligner_nativeTranslateTimestamp
      -Wl,--undefined=Java_org_webrtc_YuvHelper_nativeABGRToI420
      -Wl,--undefined=Java_org_webrtc_YuvHelper_nativeCopyPlane
      -Wl,--undefined=Java_org_webrtc_YuvHelper_nativeI420Copy
      -Wl,--undefined=Java_org_webrtc_YuvHelper_nativeI420Rotate
      -Wl,--undefined=Java_org_webrtc_YuvHelper_nativeI420ToNV12
      -Wl,--undefined=Java_org_webrtc_audio_JavaAudioDeviceModule_nativeCreateAudioDeviceModule
      -Wl,--undefined=Java_org_webrtc_BuiltinAudioEncoderFactoryFactory_nativeCreateBuiltinAudioEncoderFactory
      -Wl,--undefined=Java_org_webrtc_BuiltinAudioDecoderFactoryFactory_nativeCreateBuiltinAudioDecoderFactory
      -Wl,--undefined=Java_org_webrtc_LibvpxVp8Decoder_nativeCreateDecoder
      -Wl,--undefined=Java_org_webrtc_LibvpxVp8Encoder_nativeCreateEncoder
      -Wl,--undefined=Java_org_webrtc_NetworkMonitor_nativeNotifyConnectionTypeChanged
      -Wl,--undefined=Java_org_webrtc_NetworkMonitor_nativeNotifyOfActiveNetworkList
      -Wl,--undefined=Java_org_webrtc_NetworkMonitor_nativeNotifyOfNetworkConnect
      -Wl,--undefined=Java_org_webrtc_NetworkMonitor_nativeNotifyOfNetworkDisconnect
      -Wl,--undefined=Java_org_webrtc_JniCommon_nativeAddRef
      -Wl,--undefined=Java_org_webrtc_JniCommon_nativeAllocateByteBuffer
      -Wl,--undefined=Java_org_webrtc_JniCommon_nativeFreeByteBuffer
      -Wl,--undefined=Java_org_webrtc_JniCommon_nativeReleaseRef
      -Wl,--undefined=Java_org_webrtc_Logging_nativeEnableLogThreads
      -Wl,--undefined=Java_org_webrtc_Logging_nativeEnableLogTimeStamps
      -Wl,--undefined=Java_org_webrtc_Logging_nativeEnableLogToDebugOutput
      -Wl,--undefined=Java_org_webrtc_Logging_nativeLog
      -Wl,--undefined=Java_org_webrtc_Histogram_nativeAddSample
      -Wl,--undefined=Java_org_webrtc_Histogram_nativeCreateCounts
      -Wl,--undefined=Java_org_webrtc_Histogram_nativeCreateEnumeration
      -Wl,--undefined=Java_org_webrtc_LibvpxVp9Decoder_nativeCreateDecoder
      -Wl,--undefined=Java_org_webrtc_LibvpxVp9Decoder_nativeIsSupported
      -Wl,--undefined=Java_org_webrtc_LibvpxVp9Encoder_nativeCreateEncoder
      -Wl,--undefined=Java_org_webrtc_LibvpxVp9Encoder_nativeIsSupported
      -Wl,--undefined=Java_org_webrtc_audio_WebRtcAudioTrack_nativeCacheDirectBufferAddress
      -Wl,--undefined=Java_org_webrtc_audio_WebRtcAudioTrack_nativeGetPlayoutData
      -Wl,--undefined=Java_org_webrtc_audio_WebRtcAudioRecord_nativeCacheDirectBufferAddress
      -Wl,--undefined=Java_org_webrtc_audio_WebRtcAudioRecord_nativeDataIsRecorded
      -Wl,--undefined=Java_org_webrtc_MediaStream_nativeAddAudioTrackToNativeStream
      -Wl,--undefined=Java_org_webrtc_MediaStream_nativeAddVideoTrackToNativeStream
      -Wl,--undefined=Java_org_webrtc_MediaStream_nativeGetId
      -Wl,--undefined=Java_org_webrtc_MediaStream_nativeRemoveAudioTrack
      -Wl,--undefined=Java_org_webrtc_MediaStream_nativeRemoveVideoTrack
      -Wl,--undefined=Java_org_webrtc_RtpSender_nativeGetDtmfSender
      -Wl,--undefined=Java_org_webrtc_RtpSender_nativeGetId
      -Wl,--undefined=Java_org_webrtc_RtpSender_nativeGetParameters
      -Wl,--undefined=Java_org_webrtc_RtpSender_nativeGetStreams
      -Wl,--undefined=Java_org_webrtc_RtpSender_nativeGetTrack
      -Wl,--undefined=Java_org_webrtc_RtpSender_nativeSetFrameEncryptor
      -Wl,--undefined=Java_org_webrtc_RtpSender_nativeSetParameters
      -Wl,--undefined=Java_org_webrtc_RtpSender_nativeSetStreams
      -Wl,--undefined=Java_org_webrtc_RtpSender_nativeSetTrack
      -Wl,--undefined=Java_org_webrtc_TurnCustomizer_nativeFreeTurnCustomizer
      -Wl,--undefined=Java_org_webrtc_DtmfSender_nativeCanInsertDtmf
      -Wl,--undefined=Java_org_webrtc_DtmfSender_nativeDuration
      -Wl,--undefined=Java_org_webrtc_DtmfSender_nativeInsertDtmf
      -Wl,--undefined=Java_org_webrtc_DtmfSender_nativeInterToneGap
      -Wl,--undefined=Java_org_webrtc_DtmfSender_nativeTones
      -Wl,--undefined=Java_org_webrtc_PeerConnection_nativeAddIceCandidate
      -Wl,--undefined=Java_org_webrtc_PeerConnection_nativeAddLocalStream
      -Wl,--undefined=Java_org_webrtc_PeerConnection_nativeAddTrack
      -Wl,--undefined=Java_org_webrtc_PeerConnection_nativeAddTransceiverOfType
      -Wl,--undefined=Java_org_webrtc_PeerConnection_nativeAddTransceiverWithTrack
      -Wl,--undefined=Java_org_webrtc_PeerConnection_nativeClose
      -Wl,--undefined=Java_org_webrtc_PeerConnection_nativeConnectionState
      -Wl,--undefined=Java_org_webrtc_PeerConnection_nativeCreateAnswer
      -Wl,--undefined=Java_org_webrtc_PeerConnection_nativeCreateDataChannel
      -Wl,--undefined=Java_org_webrtc_PeerConnection_nativeCreateOffer
      -Wl,--undefined=Java_org_webrtc_PeerConnection_nativeCreatePeerConnectionObserver
      -Wl,--undefined=Java_org_webrtc_PeerConnection_nativeCreateSender
      -Wl,--undefined=Java_org_webrtc_PeerConnection_nativeFreeOwnedPeerConnection
      -Wl,--undefined=Java_org_webrtc_PeerConnection_nativeGetCertificate
      -Wl,--undefined=Java_org_webrtc_PeerConnection_nativeGetLocalDescription
      -Wl,--undefined=Java_org_webrtc_PeerConnection_nativeGetNativePeerConnection
      -Wl,--undefined=Java_org_webrtc_PeerConnection_nativeGetReceivers
      -Wl,--undefined=Java_org_webrtc_PeerConnection_nativeGetRemoteDescription
      -Wl,--undefined=Java_org_webrtc_PeerConnection_nativeGetSenders
      -Wl,--undefined=Java_org_webrtc_PeerConnection_nativeGetTransceivers
      -Wl,--undefined=Java_org_webrtc_PeerConnection_nativeIceConnectionState
      -Wl,--undefined=Java_org_webrtc_PeerConnection_nativeIceGatheringState
      -Wl,--undefined=Java_org_webrtc_PeerConnection_nativeNewGetStats
      -Wl,--undefined=Java_org_webrtc_PeerConnection_nativeOldGetStats
      -Wl,--undefined=Java_org_webrtc_PeerConnection_nativeRemoveIceCandidates
      -Wl,--undefined=Java_org_webrtc_PeerConnection_nativeRemoveLocalStream
      -Wl,--undefined=Java_org_webrtc_PeerConnection_nativeRemoveTrack
      -Wl,--undefined=Java_org_webrtc_PeerConnection_nativeSetAudioPlayout
      -Wl,--undefined=Java_org_webrtc_PeerConnection_nativeSetAudioRecording
      -Wl,--undefined=Java_org_webrtc_PeerConnection_nativeSetBitrate
      -Wl,--undefined=Java_org_webrtc_PeerConnection_nativeSetConfiguration
      -Wl,--undefined=Java_org_webrtc_PeerConnection_nativeSetLocalDescription
      -Wl,--undefined=Java_org_webrtc_PeerConnection_nativeSetRemoteDescription
      -Wl,--undefined=Java_org_webrtc_PeerConnection_nativeSignalingState
      -Wl,--undefined=Java_org_webrtc_PeerConnection_nativeStartRtcEventLog
      -Wl,--undefined=Java_org_webrtc_PeerConnection_nativeStopRtcEventLog
      -Wl,--undefined=Java_org_webrtc_PeerConnectionFactory_nativeCreateAudioSource
      -Wl,--undefined=Java_org_webrtc_PeerConnectionFactory_nativeCreateAudioTrack
      -Wl,--undefined=Java_org_webrtc_PeerConnectionFactory_nativeCreateLocalMediaStream
      -Wl,--undefined=Java_org_webrtc_PeerConnectionFactory_nativeCreatePeerConnection
      -Wl,--undefined=Java_org_webrtc_PeerConnectionFactory_nativeCreatePeerConnectionFactory
      -Wl,--undefined=Java_org_webrtc_PeerConnectionFactory_nativeCreateVideoSource
      -Wl,--undefined=Java_org_webrtc_PeerConnectionFactory_nativeCreateVideoTrack
      -Wl,--undefined=Java_org_webrtc_PeerConnectionFactory_nativeDeleteLoggable
      -Wl,--undefined=Java_org_webrtc_PeerConnectionFactory_nativeFindFieldTrialsFullName
      -Wl,--undefined=Java_org_webrtc_PeerConnectionFactory_nativeFreeFactory
      -Wl,--undefined=Java_org_webrtc_PeerConnectionFactory_nativeGetNativePeerConnectionFactory
      -Wl,--undefined=Java_org_webrtc_PeerConnectionFactory_nativeInitializeAndroidGlobals
      -Wl,--undefined=Java_org_webrtc_PeerConnectionFactory_nativeInitializeFieldTrials
      -Wl,--undefined=Java_org_webrtc_PeerConnectionFactory_nativeInitializeInternalTracer
      -Wl,--undefined=Java_org_webrtc_PeerConnectionFactory_nativeInjectLoggable
      -Wl,--undefined=Java_org_webrtc_PeerConnectionFactory_nativePrintStackTrace
      -Wl,--undefined=Java_org_webrtc_PeerConnectionFactory_nativePrintStackTracesOfRegisteredThreads
      -Wl,--undefined=Java_org_webrtc_PeerConnectionFactory_nativeShutdownInternalTracer
      -Wl,--undefined=Java_org_webrtc_PeerConnectionFactory_nativeStartAecDump
      -Wl,--undefined=Java_org_webrtc_PeerConnectionFactory_nativeStartInternalTracingCapture
      -Wl,--undefined=Java_org_webrtc_PeerConnectionFactory_nativeStopAecDump
      -Wl,--undefined=Java_org_webrtc_PeerConnectionFactory_nativeStopInternalTracingCapture
      -Wl,--undefined=Java_org_webrtc_RtcCertificatePem_nativeGenerateCertificate
      -Wl,--undefined=Java_org_webrtc_AudioTrack_nativeSetVolume
      -Wl,--undefined=Java_org_webrtc_MediaStreamTrack_nativeGetEnabled
      -Wl,--undefined=Java_org_webrtc_MediaStreamTrack_nativeGetId
      -Wl,--undefined=Java_org_webrtc_MediaStreamTrack_nativeGetKind
      -Wl,--undefined=Java_org_webrtc_MediaStreamTrack_nativeGetState
      -Wl,--undefined=Java_org_webrtc_MediaStreamTrack_nativeSetEnabled
      -Wl,--undefined=Java_org_webrtc_MediaSource_nativeGetState
      -Wl,--undefined=Java_org_webrtc_DataChannel_nativeBufferedAmount
      -Wl,--undefined=Java_org_webrtc_DataChannel_nativeClose
      -Wl,--undefined=Java_org_webrtc_DataChannel_nativeId
      -Wl,--undefined=Java_org_webrtc_DataChannel_nativeLabel
      -Wl,--undefined=Java_org_webrtc_DataChannel_nativeRegisterObserver
      -Wl,--undefined=Java_org_webrtc_DataChannel_nativeSend
      -Wl,--undefined=Java_org_webrtc_DataChannel_nativeState
      -Wl,--undefined=Java_org_webrtc_DataChannel_nativeUnregisterObserver
      -Wl,--undefined=Java_org_webrtc_RtpReceiver_nativeGetId
      -Wl,--undefined=Java_org_webrtc_RtpReceiver_nativeGetParameters
      -Wl,--undefined=Java_org_webrtc_RtpReceiver_nativeGetTrack
      -Wl,--undefined=Java_org_webrtc_RtpReceiver_nativeSetFrameDecryptor
      -Wl,--undefined=Java_org_webrtc_RtpReceiver_nativeSetObserver
      -Wl,--undefined=Java_org_webrtc_RtpReceiver_nativeUnsetObserver
      -Wl,--undefined=Java_org_webrtc_RtpTransceiver_nativeCurrentDirection
      -Wl,--undefined=Java_org_webrtc_RtpTransceiver_nativeDirection
      -Wl,--undefined=Java_org_webrtc_RtpTransceiver_nativeGetMediaType
      -Wl,--undefined=Java_org_webrtc_RtpTransceiver_nativeGetMid
      -Wl,--undefined=Java_org_webrtc_RtpTransceiver_nativeGetReceiver
      -Wl,--undefined=Java_org_webrtc_RtpTransceiver_nativeGetSender
      -Wl,--undefined=Java_org_webrtc_RtpTransceiver_nativeSetDirection
      -Wl,--undefined=Java_org_webrtc_RtpTransceiver_nativeStop
      -Wl,--undefined=Java_org_webrtc_RtpTransceiver_nativeStopped
      -Wl,--undefined=Java_org_webrtc_CallSessionFileRotatingLogSink_nativeAddSink
      -Wl,--undefined=Java_org_webrtc_CallSessionFileRotatingLogSink_nativeDeleteSink
      -Wl,--undefined=Java_org_webrtc_CallSessionFileRotatingLogSink_nativeGetLogData
  )
endif ()
