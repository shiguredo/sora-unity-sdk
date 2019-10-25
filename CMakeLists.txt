cmake_minimum_required(VERSION 3.12)

project(sora_unity_sdk C CXX)

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

endif ()

set(Boost_USE_STATIC_LIBS ON)

find_package(WebRTC REQUIRED)
find_package(Boost REQUIRED)
find_package(JSON REQUIRED)

set(_SOURCES
  src/sora_signaling.cpp
  src/unity.cpp
  src/rtc/device_video_capturer.cpp
  src/rtc/native_buffer.cpp
  src/rtc/observer.cpp
  src/rtc/rtc_connection.cpp
  src/rtc/rtc_manager.cpp
  src/rtc/scalable_track_source.cpp
)

add_library(sora_unity SHARED ${_SOURCES})
set_target_properties(sora_unity PROPERTIES CXX_STANDARD 14 C_STANDARD 99)

if (ENABLE_TSAN)
  # tsan
  set_target_properties(sora_unity PROPERTIES
    COMPILE_OPTIONS "-g;-fsanitize=thread"
    LINK_OPTIONS    "-g;-fsanitize=thread"
    INTERFACE_LINK_OPTIONS    "-g;-fsanitize=thread")
endif()

if (ENABLE_ASAN)
  # asan
  set_target_properties(sora_unity PROPERTIES
    COMPILE_FLAGS "-g;-fsanitize=address;-fno-omit-frame-pointer"
    LINK_OPTIONS  "-g;-fsanitize=address;-fno-omit-frame-pointer"
    INTERFACE_LINK_OPTIONS  "-g;-fsanitize=address;-fno-omit-frame-pointer")
endif()

target_link_libraries(sora_unity
  WebRTC::WebRTC
  Boost::boost
  JSON::JSON
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
target_compile_definitions(sora_unity PRIVATE
  NDEBUG
  _CONSOLE
  _WIN32_WINNT=0x0A00
  WEBRTC_WIN
  NOMINMAX
  WIN32_LEAN_AND_MEAN
)