cmake_minimum_required(VERSION 3.12)

project(SoraUnitySdk C CXX)

if (MSVC)
  # 文字コードを utf-8 として扱うのと、シンボルテーブル数を増やす
  add_compile_options("/utf-8;/bigobj")
  add_definitions(-DUNICODE -D_UNICODE)

  # CRTライブラリを静的リンクさせる
  # https://sgry.jp/blog/2017/05/27/4465/
  string(REPLACE "/MD" "/MT" CMAKE_C_FLAGS_DEBUG            ${CMAKE_C_FLAGS_DEBUG})
  string(REPLACE "/MD" "/MT" CMAKE_C_FLAGS_MINSIZEREL       ${CMAKE_C_FLAGS_MINSIZEREL})
  string(REPLACE "/MD" "/MT" CMAKE_C_FLAGS_RELEASE          ${CMAKE_C_FLAGS_RELEASE})
  string(REPLACE "/MD" "/MT" CMAKE_C_FLAGS_RELWITHDEBINFO   ${CMAKE_C_FLAGS_RELWITHDEBINFO})
  string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_DEBUG          ${CMAKE_CXX_FLAGS_DEBUG})
  string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_MINSIZEREL     ${CMAKE_CXX_FLAGS_MINSIZEREL})
  string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELEASE        ${CMAKE_CXX_FLAGS_RELEASE})
  string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})

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
    wmcodecdspuuid.lib)

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

configure_file(src/sora_version.h.template ${CMAKE_CURRENT_BINARY_DIR}/sora_version.h)
set(_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_BINARY_DIR})

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
)

if (WIN32)
  list(APPEND _SOURCES
    src/unity_camera_capturer_d3d11.cpp)

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
