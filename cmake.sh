#!/bin/bash

PROGRAM="$0"

_PACKAGES=" \
  macos \
  android \
  ios \
"

set -e

function show_help() {
  echo "$PROGRAM <package>"
  echo "<package>:"
  for package in $_PACKAGES; do
    echo "  - $package"
  done
}

if [ $# -lt 1 ]; then
  show_help
  exit 1
fi

if [ -z "$JOBS" ]; then
  set +e
  JOBS=`nproc`
  if [ -z "$JOBS" ]; then
    JOBS=`sysctl -n hw.logicalcpu_max`
    if [ -z "$JOBS" ]; then
      JOBS=1
    fi
  fi
  set -e
fi

PACKAGE="$1"

_FOUND=0
for package in $_PACKAGES; do
  if [ "$PACKAGE" = "$package" ]; then
    _FOUND=1
    break
  fi
done

if [ $_FOUND -eq 0 ]; then
  show_help
  exit 1
fi

set -x

INSTALL_DIR="`pwd`/_install"
MODULE_PATH="`pwd`/cmake"

source "$INSTALL_DIR/$PACKAGE/webrtc/VERSIONS"
source "`pwd`/VERSIONS"
SORA_UNITY_SDK_COMMIT="`git rev-parse HEAD`"

CMAKE_ARGS=""
CMAKE_BUILD_ARGS=""

if [ "$PACKAGE" = "ios" ]; then
  CMAKE_ARGS=" \
    -G Xcode \
    -DCMAKE_SYSTEM_NAME=iOS \
    -DCMAKE_OSX_ARCHITECTURES=arm64;x86_64 \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=10.0 \
    -DCMAKE_XCODE_ATTRIBUTE_ONLY_ACTIVE_ARCH=NO \
    -DCMAKE_IOS_INSTALL_COMBINED=YES \
    -DCMAKE_INSTALL_PREFIX=`pwd`/build/$PACKAGE/_install
  "
  CMAKE_BUILD_ARGS="--config Release --target install"
fi

mkdir -p build/$PACKAGE
pushd build/$PACKAGE
  cmake ../.. \
    -DCMAKE_BUILD_TYPE=Release \
    -DSORA_UNITY_SDK_PACKAGE="$PACKAGE" \
    -DSORA_UNITY_SDK_VERSION="$SORA_UNITY_SDK_VERSION" \
    -DSORA_UNITY_SDK_COMMIT="$SORA_UNITY_SDK_COMMIT" \
    -DWEBRTC_BUILD_VERSION="$WEBRTC_BUILD_VERSION" \
    -DWEBRTC_READABLE_VERSION="$WEBRTC_READABLE_VERSION" \
    -DWEBRTC_SRC_COMMIT="$WEBRTC_SRC_COMMIT" \
    -DWEBRTC_ROOT_DIR="$INSTALL_DIR/$PACKAGE/webrtc" \
    -DJSON_ROOT_DIR"=$INSTALL_DIR/json" \
    -DCMAKE_MODULE_PATH="$MODULE_PATH" \
    -DCMAKE_PREFIX_PATH="$INSTALL_DIR/boost" \
    -DANDROID_TOOLCHAIN_FILE="$INSTALL_DIR/android-ndk/build/cmake/android.toolchain.cmake" \
    $CMAKE_ARGS
  cmake --build . -j$JOBS $CMAKE_BUILD_ARGS
popd
