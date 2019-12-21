#!/bin/bash

set -ex

BUILD_DIR="`pwd`/_build"
INSTALL_DIR="`pwd`/_install"

source `pwd`/VERSIONS

mkdir -p $BUILD_DIR
mkdir -p $INSTALL_DIR

# WebRTC のインストール
if [ ! -e $INSTALL_DIR/webrtc/lib/libwebrtc.a ]; then
  # shiguredo-webrtc-build からバイナリをダウンロードして配置するだけ
  pushd $BUILD_DIR
    rm -rf webrtc.macos.tar.gz
    curl -LO https://github.com/shiguredo-webrtc-build/webrtc-build/releases/download/m${WEBRTC_VERSION}/webrtc.macos.tar.gz
  popd

  pushd $INSTALL_DIR
    rm -rf webrtc/
    tar xf $BUILD_DIR/webrtc.macos.tar.gz
  popd
fi

# nlohmann/json
if [ ! -e $INSTALL_DIR/json/include ]; then
  rm -rf $INSTALL_DIR/json
  git clone --branch v$JSON_VERSION --depth 1 https://github.com/nlohmann/json.git $INSTALL_DIR/json
fi

# Boost
if [ ! -e $INSTALL_DIR/boost/lib/libboost_filesystem.a ]; then
  _VERSION_UNDERSCORE=${BOOST_VERSION//./_}
  _URL=https://dl.bintray.com/boostorg/release/${BOOST_VERSION}/source/boost_${_VERSION_UNDERSCORE}.tar.gz
  _FILE=$BUILD_DIR/boost_${_VERSION_UNDERSCORE}.tar.gz
  if [ ! -e $_FILE ]; then
    echo "file(DOWNLOAD $_URL $_FILE)" > $BUILD_DIR/tmp.cmake
    cmake -P $BUILD_DIR/tmp.cmake
    rm $BUILD_DIR/tmp.cmake
  fi
  pushd $BUILD_DIR
    rm -rf boost_${_VERSION_UNDERSCORE}
    cmake -E tar xf $_FILE

    pushd boost_${_VERSION_UNDERSCORE}
      ./bootstrap.sh
      ./b2 install --prefix=$INSTALL_DIR/boost --build-dir=build link=static --with-filesystem
    popd
  popd
fi
