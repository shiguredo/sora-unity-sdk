cmake_minimum_required(VERSION 3.16)

# Only interpret if() arguments as variables or keywords when unquoted.
cmake_policy(SET CMP0054 NEW)
# MSVC runtime library flags are selected by an abstraction.
cmake_policy(SET CMP0091 NEW)

project(SoraUnitySdk C CXX)

list(APPEND CMAKE_PREFIX_PATH ${SORA_DIR})
list(APPEND CMAKE_MODULE_PATH ${SORA_DIR}/share/cmake)

set(Boost_USE_STATIC_LIBS ON)
if (WIN32)
  set(Boost_USE_STATIC_RUNTIME ON)
endif()

set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE NEVER)

find_package(Boost REQUIRED COMPONENTS json filesystem)
find_package(WebRTC REQUIRED)
find_package(Sora REQUIRED)
find_package(Threads REQUIRED)

set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

if (SORA_UNITY_SDK_PACKAGE STREQUAL "windows_x86_64")
  add_library(SoraUnitySdk SHARED)

  # configure_file で使う
  set(SORA_UNITY_SDK_PLATFORM Windows)
elseif (SORA_UNITY_SDK_PACKAGE STREQUAL "macos_x86_64" OR SORA_UNITY_SDK_PACKAGE STREQUAL "macos_arm64")
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
elseif (SORA_UNITY_SDK_PACKAGE STREQUAL "ubuntu-22.04_x86_64")
  add_library(SoraUnitySdk SHARED)

  set(SORA_UNITY_SDK_PLATFORM Ubuntu)
endif()

set_target_properties(SoraUnitySdk PROPERTIES CXX_STANDARD 20 C_STANDARD 99)

target_sources(SoraUnitySdk
  PRIVATE
    src/converter.cpp
    src/device_list.cpp
    src/id_pointer.cpp
    src/sora.cpp
    src/unity_camera_capturer.cpp
    src/unity_context.cpp
    src/unity_renderer.cpp
    src/unity.cpp
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
    "${CMAKE_CURRENT_BINARY_DIR}/proto/cpp/sora_conf_internal.json.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Sora/Generated/SoraConf.cs"
    "${CMAKE_CURRENT_SOURCE_DIR}/Sora/Generated/SoraConfInternal.cs"
    "${CMAKE_CURRENT_SOURCE_DIR}/Sora/Generated/Jsonif.cs"
  COMMAND
    "${PROTOBUF_DIR}/bin/protoc${EXE_EXT}"
  ARGS
    --jsonif-cpp_out "${CMAKE_CURRENT_BINARY_DIR}/proto/cpp"
    --jsonif-unity_out "${CMAKE_CURRENT_SOURCE_DIR}/Sora/Generated"
    --plugin=protoc-gen-jsonif-cpp="${PROTOC_GEN_JSONIF_DIR}/bin/protoc-gen-jsonif-cpp${EXE_EXT}"
    --plugin=protoc-gen-jsonif-unity="${PROTOC_GEN_JSONIF_DIR}/bin/protoc-gen-jsonif-unity${EXE_EXT}"
    -I "${CMAKE_CURRENT_SOURCE_DIR}/proto/"
    "${CMAKE_CURRENT_SOURCE_DIR}/proto/sora_conf.proto"
    "${CMAKE_CURRENT_SOURCE_DIR}/proto/sora_conf_internal.proto"
  DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/proto/sora_conf.proto"
    "${CMAKE_CURRENT_SOURCE_DIR}/proto/sora_conf_internal.proto"
)
add_custom_target(sora_conf.json.h
  DEPENDS
    "${CMAKE_CURRENT_BINARY_DIR}/proto/cpp/sora_conf.json.h"
    "${CMAKE_CURRENT_BINARY_DIR}/proto/cpp/sora_conf_internal.json.h"
)
add_dependencies(SoraUnitySdk sora_conf.json.h)
target_include_directories(SoraUnitySdk PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/proto/cpp")

string(SUBSTRING ${SORA_UNITY_SDK_COMMIT} 0 8 SORA_UNITY_SDK_COMMIT_SHORT)
string(SUBSTRING ${WEBRTC_COMMIT} 0 8 WEBRTC_COMMIT_SHORT)

configure_file(src/sora_version.h.template ${CMAKE_CURRENT_BINARY_DIR}/sora_version.h)

target_include_directories(SoraUnitySdk PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(SoraUnitySdk PRIVATE Sora::sora)

if (SORA_UNITY_SDK_PACKAGE STREQUAL "windows_x86_64")
  # 文字コードを utf-8 として扱うのと、シンボルテーブル数を増やす
  target_compile_options(SoraUnitySdk
    PRIVATE
      "$<$<COMPILE_LANGUAGE:CXX>:/utf-8;/bigobj>"
  )
  set_target_properties(SoraUnitySdk
    PROPERTIES
      # CRTライブラリを静的リンクさせる
      MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
  )

  target_sources(SoraUnitySdk PRIVATE src/unity_camera_capturer_d3d11.cpp)

  #target_link_libraries(SoraUnitySdk
  #  PRIVATE
  #    dbghelp.lib
  #    delayimp.lib
  #    dnsapi.lib
  #    msimg32.lib
  #    oleaut32.lib
  #    psapi.lib
  #    shell32.lib
  #    shlwapi.lib
  #    usp10.lib
  #    version.lib
  #    wininet.lib
  #    winmm.lib
  #    ws2_32.lib
  #    amstrmid.lib
  #    Strmiids.lib
  #    crypt32.lib
  #    dmoguids.lib
  #    iphlpapi.lib
  #    msdmo.lib
  #    Secur32.lib
  #    wmcodecdspuuid.lib
  #    dxgi.lib
  #    D3D11.lib
  #)

  target_compile_definitions(SoraUnitySdk
    PRIVATE
      SORA_UNITY_SDK_WINDOWS
      _WIN32_WINNT=0x0A00
      NOMINMAX
      WIN32_LEAN_AND_MEAN
  )
elseif (SORA_UNITY_SDK_PACKAGE STREQUAL "macos_x86_64" OR SORA_UNITY_SDK_PACKAGE STREQUAL "macos_arm64")
  enable_language(OBJCXX)

  target_sources(SoraUnitySdk PRIVATE src/unity_camera_capturer_metal.mm)

  #target_link_libraries(SoraUnitySdk
  #  PRIVATE
  #    "-framework Foundation"
  #    "-framework AVFoundation"
  #    "-framework CoreServices"
  #    "-framework CoreFoundation"
  #    "-framework AudioUnit"
  #    "-framework AudioToolbox"
  #    "-framework CoreAudio"
  #    "-framework CoreGraphics"
  #    "-framework CoreMedia"
  #    "-framework CoreVideo"
  #    "-framework QuartzCore"
  #    "-framework VideoToolbox"
  #    "-framework AppKit"
  #    "-framework Metal"
  #    "-framework MetalKit"
  #    "-framework OpenGL"
  #)

  target_compile_definitions(SoraUnitySdk
    PRIVATE
      SORA_UNITY_SDK_MACOS
  )

elseif (SORA_UNITY_SDK_PACKAGE STREQUAL "ios")
  enable_language(OBJCXX)

  target_sources(SoraUnitySdk
    PRIVATE
      src/unity_camera_capturer_metal.mm
      src/mac_helper/ios_audio_init.mm
  )

  target_compile_definitions(SoraUnitySdk
    PRIVATE
      SORA_UNITY_SDK_IOS
  )
  install(TARGETS SoraUnitySdk DESTINATION lib)

elseif (SORA_UNITY_SDK_PACKAGE STREQUAL "android")

  # ref: https://github.com/shiguredo/sora-cpp-sdk/blob/bfb7ed87f4d4c08212115dce00f4be6629ea65d4/test/android/app/src/main/cpp/CMakeLists.txt#L48-L56
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -B ${CLANG_DIR}/bin")

  find_library(ANDROID_LIB_LOG log)
  find_library(ANDROID_LIB_ANDROID android)
  find_library(ANDROID_LIB_OPENSLES OpenSLES)
  find_library(ANDROID_LIB_VULKAN vulkan)
  find_library(ANDROID_LIB_EGL EGL)
  find_library(ANDROID_LIB_GLESV2 GLESv2)

  target_sources(SoraUnitySdk
    PRIVATE
      src/android_helper/jni_onload.cc
      src/android_helper/android_context.cpp
      src/unity_camera_capturer_vulkan.cpp
      src/unity_camera_capturer_opengl.cpp
  )

  target_compile_definitions(SoraUnitySdk
    PRIVATE
      SORA_UNITY_SDK_ANDROID
  )
  target_compile_options(SoraUnitySdk PRIVATE "-nostdinc++")
  target_include_directories(SoraUnitySdk PRIVATE ${LIBCXX_INCLUDE_DIR})

  target_link_libraries(SoraUnitySdk
    PRIVATE
      ${ANDROID_LIB_LOG}
      ${ANDROID_LIB_ANDROID}
      ${ANDROID_LIB_OPENSLES}
      ${ANDROID_LIB_VULKAN}
      ${ANDROID_LIB_EGL}
      ${ANDROID_LIB_GLESV2}
  )
  file(READ ${SORA_DIR}/share/webrtc.ldflags _WEBRTC_ANDROID_LDFLAGS)
  string(REGEX REPLACE "\n" ";" _WEBRTC_ANDROID_LDFLAGS "${_WEBRTC_ANDROID_LDFLAGS}")
  target_link_options(SoraUnitySdk
    PRIVATE
      ${_WEBRTC_ANDROID_LDFLAGS}
  )

elseif (SORA_UNITY_SDK_PACKAGE STREQUAL "ubuntu-22.04_x86_64")

  target_sources(SoraUnitySdk
    PRIVATE
      src/unity_camera_capturer_opengl.cpp
  )

  target_compile_definitions(SoraUnitySdk PRIVATE SORA_UNITY_SDK_UBUNTU)
  target_compile_options(SoraUnitySdk PRIVATE "-nostdinc++")
  target_include_directories(SoraUnitySdk PRIVATE ${LIBCXX_INCLUDE_DIR})

endif()
