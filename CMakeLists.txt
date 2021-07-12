cmake_minimum_required(VERSION 3.16)

# Only interpret if() arguments as variables or keywords when unquoted.
cmake_policy(SET CMP0054 NEW)
# MSVC runtime library flags are selected by an abstraction.
cmake_policy(SET CMP0091 NEW)

if (SORA_UNITY_SDK_PACKAGE STREQUAL "android")
  # toolchain の設定
  set(CMAKE_TOOLCHAIN_FILE ${ANDROID_TOOLCHAIN_FILE})
  set(ANDROID_NATIVE_API_LEVEL 24)
  set(ANDROID_ABI arm64-v8a)
  set(ANDROID_STL none)
endif()

set(_INSTALL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/_install)

project(SoraUnitySdk C CXX)

set(Boost_USE_STATIC_LIBS ON)

set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)

find_package(WebRTC REQUIRED)
find_package(Boost REQUIRED)

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
elseif (SORA_UNITY_SDK_PACKAGE STREQUAL "ios")
  add_library(SoraUnitySdk)
  set_target_properties(SoraUnitySdk PROPERTIES CXX_VISIBILITY_PRESET hidden)
  target_link_options(SoraUnitySdk PRIVATE -ObjC)

  set(SORA_UNITY_SDK_PLATFORM iOS)
elseif (SORA_UNITY_SDK_PACKAGE STREQUAL "android")
  add_library(SoraUnitySdk SHARED)

  set(SORA_UNITY_SDK_PLATFORM Android)
endif()

set_target_properties(SoraUnitySdk PROPERTIES CXX_STANDARD 14 C_STANDARD 99)

target_sources(SoraUnitySdk
  PRIVATE
    src/boost_json.cpp
    src/id_pointer.cpp
    src/sora.cpp
    src/sora_signaling.cpp
    src/ssl_verifier.cpp
    src/unity.cpp
    src/unity_camera_capturer.cpp
    src/unity_context.cpp
    src/unity_renderer.cpp
    src/watchdog.cpp
    src/websocket.cpp
    src/rtc/device_list.cpp
    src/rtc/device_video_capturer.cpp
    src/rtc/native_buffer.cpp
    src/rtc/peer_connection_observer.cpp
    src/rtc/rtc_connection.cpp
    src/rtc/rtc_manager.cpp
    src/rtc/rtc_ssl_verifier.cpp
    src/rtc/scalable_track_source.cpp
    src/rtc/simulcast_video_encoder_factory.cpp
)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/proto")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/proto/cpp")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Sora/Generated")
if (WIN32)
  set(EXE_EXT ".exe")
endif()
add_custom_command(
  OUTPUT
    "${CMAKE_CURRENT_BINARY_DIR}/proto/cpp/sora_conf.json.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Sora/Generated/SoraConf.cs"
    "${CMAKE_CURRENT_SOURCE_DIR}/Sora/Generated/Jsonif.cs"
  COMMAND
    "${_INSTALL_DIR}/protoc/bin/protoc${EXE_EXT}"
  ARGS
    --jsonif-cpp_out "${CMAKE_CURRENT_BINARY_DIR}/proto/cpp"
    --jsonif-unity_out "${CMAKE_CURRENT_SOURCE_DIR}/Sora/Generated"
    --plugin=protoc-gen-jsonif-cpp="${_INSTALL_DIR}/protoc-gen-jsonif/bin/protoc-gen-jsonif-cpp${EXE_EXT}"
    --plugin=protoc-gen-jsonif-unity="${_INSTALL_DIR}/protoc-gen-jsonif/bin/protoc-gen-jsonif-unity${EXE_EXT}"
    -I "${CMAKE_CURRENT_SOURCE_DIR}/proto/"
    "${CMAKE_CURRENT_SOURCE_DIR}/proto/sora_conf.proto"
  MAIN_DEPENDENCY
    "${CMAKE_CURRENT_SOURCE_DIR}/proto/sora_conf.proto"
)
add_custom_target(sora_conf.json.h DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/proto/cpp/sora_conf.json.h")
add_dependencies(SoraUnitySdk sora_conf.json.h)
target_include_directories(SoraUnitySdk PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/proto/cpp")

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
      -Xcompiler /I${CMAKE_CURRENT_SOURCE_DIR}/src
  )
  target_sources(SoraUnitySdk PRIVATE ${CUDA_FILES})
  target_include_directories(SoraUnitySdk PRIVATE ${CUDA_INCLUDE_DIRS})
  target_link_libraries(SoraUnitySdk
    PRIVATE
      #${CUDA_TOOLKIT_ROOT_DIR}/lib/x64/cuda.lib
      ${CUDA_LIBRARIES}
      #${CMAKE_CURRENT_SOURCE_DIR}/NvCodec/Lib/x64/nvcuvid.lib
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
      "-framework QuartzCore"
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

elseif (SORA_UNITY_SDK_PACKAGE STREQUAL "ios")
  enable_language(OBJCXX)

  target_sources(SoraUnitySdk
    PRIVATE
      src/unity_camera_capturer_metal.mm
      src/mac_helper/mac_capturer.mm
      src/mac_helper/objc_codec_factory_helper.mm
      src/mac_helper/ios_audio_init.mm
  )

  target_compile_definitions(SoraUnitySdk
    PRIVATE
      SORA_UNITY_SDK_IOS
      WEBRTC_POSIX
      WEBRTC_MAC
      WEBRTC_IOS
  )
  install(TARGETS SoraUnitySdk DESTINATION lib)

elseif (SORA_UNITY_SDK_PACKAGE STREQUAL "android")

  find_library(ANDROID_LIB_LOG log)
  find_library(ANDROID_LIB_ANDROID android)
  find_library(ANDROID_LIB_OPENSLES OpenSLES)
  find_library(ANDROID_LIB_VULKAN vulkan)
  find_library(ANDROID_LIB_EGL EGL)

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
      _LIBCPP_DISABLE_AVAILABILITY
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
      ${ANDROID_LIB_EGL}
  )
  file(READ ${_INSTALL_DIR}/android/webrtc.ldflags _WEBRTC_ANDROID_LDFLAGS)
  string(REGEX REPLACE "\n" ";" _WEBRTC_ANDROID_LDFLAGS "${_WEBRTC_ANDROID_LDFLAGS}")
  target_link_options(SoraUnitySdk
    PRIVATE
      -fuse-ld=gold
      #-Wl,--wrap=malloc
      #-Wl,--wrap=calloc
      #-Wl,--wrap=memalign
      #-Wl,--wrap=realloc
      #-Wl,--wrap=free
      #-Wl,--wrap=realpath
      #-Wl,--wrap=getcwd
      ${_WEBRTC_ANDROID_LDFLAGS}
  )
endif ()
