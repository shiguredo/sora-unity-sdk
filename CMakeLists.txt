cmake_minimum_required(VERSION 3.12)

cmake_policy(SET CMP0091 NEW)

project(SoraUnitySdk C CXX)

if (MSVC)
  # 文字コードを utf-8 として扱うのと、シンボルテーブル数を増やす
  add_compile_options("/utf-8;/bigobj")
  add_definitions(-DUNICODE -D_UNICODE)

  # CRTライブラリを静的リンクさせる
  # CMake 3.15 以上なら CMAKE_MSVC_RUNTIME_LIBRARY が使える
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

  set(_LINK_LIBRARIES
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
    D3D11.lib)

  set(_COMPILE_DEFINITIONS
    _CONSOLE
    _WIN32_WINNT=0x0A00
    WEBRTC_WIN
    NOMINMAX
    WIN32_LEAN_AND_MEAN)

  # configure_file で使う
  set(SORA_UNITY_SDK_PLATFORM Windows)
elseif (APPLE)

  set(_LINK_LIBRARIES
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
    "-framework OpenGL")

  set(_COMPILE_DEFINITIONS
    WEBRTC_POSIX
    WEBRTC_MAC)

  # configure_file で使う
  set(SORA_UNITY_SDK_PLATFORM macOS)
endif ()

string(SUBSTRING ${SORA_UNITY_SDK_COMMIT} 0 8 SORA_UNITY_SDK_COMMIT_SHORT)
string(SUBSTRING ${WEBRTC_SRC_COMMIT} 0 8 WEBRTC_SRC_COMMIT_SHORT)

set(_INCLUDE_DIRECTORIES)

configure_file(src/sora_version.h.template ${CMAKE_CURRENT_BINARY_DIR}/sora_version.h)
list(APPEND _INCLUDE_DIRECTORIES ${CMAKE_CURRENT_BINARY_DIR})

list(APPEND _INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/src)
list(APPEND _INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/NvCodec/include)
list(APPEND _INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/NvCodec/NvCodec)

set(Boost_USE_STATIC_LIBS ON)

find_package(WebRTC REQUIRED)
find_package(Boost REQUIRED)
find_package(JSON REQUIRED)

set(_SOURCES
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
  src/hwenc_nvcodec/nvcodec_h264_encoder.cpp
)

if (WIN32)
  list(APPEND _SOURCES
    src/unity_camera_capturer_d3d11.cpp
    NvCodec/NvCodec/NvEncoder/NvEncoder.cpp
    NvCodec/NvCodec/NvEncoder/NvEncoderD3D11.cpp)

  add_library(SoraUnitySdk SHARED ${_SOURCES})
elseif (APPLE)
  list(APPEND _SOURCES
    src/unity_camera_capturer_metal.mm
    src/mac_helper/mac_capturer.mm
    src/mac_helper/objc_codec_factory_helper.mm)

  # Objective-C++ の設定
  set_source_files_properties(src/unity_camera_capturer.mm PROPERTIES
    COMPILE_OPTIONS "-x;objective-c++;-fmodules;-fcxx-modules")
  set_source_files_properties(src/mac_helper/mac_capturer.mm PROPERTIES
    COMPILE_OPTIONS "-x;objective-c++;-fmodules;-fcxx-modules")
  set_source_files_properties(src/mac_helper/objc_codec_factory_helper.mm PROPERTIES
    COMPILE_OPTIONS "-x;objective-c++;-fmodules;-fcxx-modules")

  add_library(SoraUnitySdk MODULE ${_SOURCES})
  set_target_properties(SoraUnitySdk PROPERTIES BUNDLE TRUE)
  set_target_properties(SoraUnitySdk PROPERTIES CXX_VISIBILITY_PRESET hidden)
endif ()

set_target_properties(SoraUnitySdk PROPERTIES CXX_STANDARD 14 C_STANDARD 99)

target_include_directories(SoraUnitySdk PRIVATE ${_INCLUDE_DIRECTORIES})

if (ENABLE_TSAN)
  # tsan
  set_target_properties(SoraUnitySdk PROPERTIES
    COMPILE_OPTIONS "-g;-fsanitize=thread"
    LINK_OPTIONS    "-g;-fsanitize=thread"
    INTERFACE_LINK_OPTIONS    "-g;-fsanitize=thread")
endif()

if (ENABLE_ASAN)
  # asan
  set_target_properties(SoraUnitySdk PROPERTIES
    COMPILE_FLAGS "-g;-fsanitize=address;-fno-omit-frame-pointer"
    LINK_OPTIONS  "-g;-fsanitize=address;-fno-omit-frame-pointer"
    INTERFACE_LINK_OPTIONS  "-g;-fsanitize=address;-fno-omit-frame-pointer")
endif()

target_link_libraries(SoraUnitySdk
  WebRTC::WebRTC
  Boost::boost
  JSON::JSON
  ${_LINK_LIBRARIES})

target_compile_definitions(SoraUnitySdk PRIVATE
  NDEBUG
  ${_COMPILE_DEFINITIONS})
