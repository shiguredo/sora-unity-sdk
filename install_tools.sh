#!/bin/bash

set -ex

BUILD_DIR="`pwd`/_build"
INSTALL_DIR="`pwd`/_install"

source `pwd`/VERSIONS

mkdir -p $BUILD_DIR
mkdir -p $INSTALL_DIR

# WebRTC のインストール
if [ ! -e $INSTALL_DIR/webrtc/lib/libwebrtc.a ]; then
  # shiguredo-webrtc-build から各環境のバイナリをダウンロードして配置するだけ
  for name in macos; do
    pushd $BUILD_DIR
      rm -rf webrtc.$name.tar.gz
      curl -LO https://github.com/shiguredo-webrtc-build/webrtc-build/releases/download/m${WEBRTC_BUILD_VERSION}/webrtc.$name.tar.gz
    popd

    pushd $INSTALL_DIR/$name
      rm -rf webrtc/
      tar xf $BUILD_DIR/webrtc.$name.tar.gz
    popd
  done
fi

# nlohmann/json
if [ ! -e $INSTALL_DIR/json/include ]; then
  rm -rf $INSTALL_DIR/json
  git clone --branch v$JSON_VERSION --depth 1 https://github.com/nlohmann/json.git $INSTALL_DIR/json
fi

# Boost
if [ ! -e $INSTALL_DIR/boost/include/boost/version.hpp ]; then
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
      #./b2 install --prefix=$INSTALL_DIR/boost --build-dir=build link=static --with-filesystem
      ./b2 install --prefix=$INSTALL_DIR/boost --build-dir=build
    popd
  popd
fi

# Android NDK のインストール
if [ ! -e $INSTALL_DIR/android-ndk ]; then
  _URL=https://dl.google.com/android/repository/android-ndk-${ANDROID_NDK_VERSION}-darwin-x86_64.zip
  _FILE=$BUILD_DIR/android-ndk-${ANDROID_NDK_VERSION}-darwin-x86_64.zip
  mkdir -p $BUILD_DIR
  if [ ! -e $_FILE ]; then
    echo "file(DOWNLOAD $_URL $_FILE)" > $BUILD_DIR/tmp.cmake
    cmake -P $BUILD_DIR/tmp.cmake
    rm $BUILD_DIR/tmp.cmake
  fi
  pushd $INSTALL_DIR
    rm -rf android-ndk
    rm -rf android-ndk-${ANDROID_NDK_VERSION}
    cmake -E tar xf $_FILE
    mv android-ndk-${ANDROID_NDK_VERSION} android-ndk
  popd
fi

# 特定バージョンの libcxx, libcxxabi を取得
source $INSTALL_DIR/macos/webrtc/VERSIONS
pushd $INSTALL_DIR
  if [ ! -e libcxx/.git ]; then
    git clone https://chromium.googlesource.com/chromium/llvm-project/libcxx
  fi
  pushd libcxx
    git fetch
    git reset --hard $WEBRTC_SRC_BUILDTOOLS_THIRD_PARTY_LIBCXX_TRUNK
  popd

  if [ ! -e libcxxabi/.git ]; then
    git clone https://chromium.googlesource.com/chromium/llvm-project/libcxxabi
  fi
  pushd libcxxabi
    git fetch
    git reset --hard $WEBRTC_SRC_BUILDTOOLS_THIRD_PARTY_LIBCXXABI_TRUNK
  popd
popd
