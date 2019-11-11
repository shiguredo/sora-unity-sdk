#!/bin/bash

INSTALL_DIR="`pwd`/_install"
MODULE_PATH="`pwd`/cmake"

mkdir -p build
pushd build
  cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DWEBRTC_ROOT_DIR="$INSTALL_DIR/webrtc" \
    -DJSON_ROOT_DIR"=$INSTALL_DIR/json" \
    -DCMAKE_MODULE_PATH="$MODULE_PATH" \
    -DCMAKE_PREFIX_PATH="$INSTALL_DIR/boost"
  cmake --build .
popd
