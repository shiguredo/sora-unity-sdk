cmake_minimum_required(VERSION 3.15)

# Only interpret if() arguments as variables or keywords when unquoted.
cmake_policy(SET CMP0054 NEW)
# MSVC runtime library flags are selected by an abstraction.
cmake_policy(SET CMP0091 NEW)

project(SoraUnitySdk C CXX)

set(Boost_USE_STATIC_LIBS ON)

find_package(WebRTC REQUIRED)
find_package(Boost REQUIRED)
find_package(JSON REQUIRED)

if (MSVC)
  add_library(SoraUnitySdk SHARED)

  # configure_file で使う
  set(SORA_UNITY_SDK_PLATFORM Windows)
elseif (APPLE)
  add_library(SoraUnitySdk MODULE)
  set_target_properties(SoraUnitySdk PROPERTIES BUNDLE TRUE)
  set_target_properties(SoraUnitySdk PROPERTIES CXX_VISIBILITY_PRESET hidden)
  target_link_options(SoraUnitySdk PRIVATE -ObjC)

  # configure_file で使う
  set(SORA_UNITY_SDK_PLATFORM macOS)
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
    src/rtc/hw_video_encoder_factory.cpp
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

if (MSVC)
  # 文字コードを utf-8 として扱うのと、シンボルテーブル数を増やす
  target_compile_options(SoraUnitySdk PRIVATE /utf-8 /bigobj)
  set_target_properties(SoraUnitySdk
    PROPERTIES
      # CRTライブラリを静的リンクさせる
      # CMake 3.15 以上なら MSVC_RUNTIME_LIBRARY が使える
      MSVC_RUNTIME_LIBRARY "MultiThreaded"
  )

  target_sources(SoraUnitySdk
    PRIVATE
      src/unity_camera_capturer_d3d11.cpp
      src/hwenc_nvcodec/nvcodec_h264_encoder.cpp
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
      UNICODE
      _UNICODE
      _CONSOLE
      _WIN32_WINNT=0x0A00
      WEBRTC_WIN
      NOMINMAX
      WIN32_LEAN_AND_MEAN
  )
elseif (APPLE)
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
      WEBRTC_POSIX
      WEBRTC_MAC
  )
endif ()
