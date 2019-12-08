#!/bin/bash

set -ex

BUILD_DIR="`pwd`/_build"
INSTALL_DIR="`pwd`/_install"

source `pwd`/VERSIONS

mkdir -p $BUILD_DIR
mkdir -p $INSTALL_DIR

# WebRTC のインストール
if [ ! -e $INSTALL_DIR/webrtc/lib/libwebrtc.a ]; then
  # Momo を clone して macOS 用ビルドを行い、その過程で生成された
  # WebRTC のライブラリをこのプロジェクトにコピーする

  if [ -e $BUILD_DIR/momo ]; then
    pushd $BUILD_DIR/momo
      git pull
    popd
  else
    git clone https://github.com/shiguredo/momo.git $BUILD_DIR/momo
  fi

  pushd $BUILD_DIR/momo/build
    make macos
  popd

  rm -rf $INSTALL_DIR/webrtc

  # libwebrtc.a
  mkdir -p $INSTALL_DIR/webrtc/lib
  cp $BUILD_DIR/momo/_build/macos/libwebrtc.a $INSTALL_DIR/webrtc/lib

  # ヘッダ類
  mkdir -p $INSTALL_DIR/webrtc/include
  rsync -amv --include="*/" --include="*.h" --include="*.hpp" --exclude="*" $BUILD_DIR/momo/build/macos/webrtc/src/. $INSTALL_DIR/webrtc/include/.

  # 各種情報を拾ってくる
  touch $INSTALL_DIR/webrtc/VERSIONS
  pushd $BUILD_DIR/momo/build/macos/webrtc/src
    echo "WEBRTC_SRC_COMMIT=`git rev-parse HEAD`" >> $INSTALL_DIR/webrtc/VERSIONS
  popd
  pushd $BUILD_DIR/momo/build/macos/webrtc/src/build
    echo "WEBRTC_SRC_BUILD_COMMIT=`git rev-parse HEAD`" >> $INSTALL_DIR/webrtc/VERSIONS
  popd
  pushd $BUILD_DIR/momo/build/macos/webrtc/src/buildtools
    echo "WEBRTC_SRC_BUILDTOOLS_COMMIT=`git rev-parse HEAD`" >> $INSTALL_DIR/webrtc/VERSIONS
  popd
  pushd $BUILD_DIR/momo/build/macos/webrtc/src/buildtools/third_party/libc++/trunk
    echo "WEBRTC_SRC_BUILDTOOLS_THIRD_PARTY_LIBCXX_TRUNK=`git rev-parse HEAD`" >> $INSTALL_DIR/webrtc/VERSIONS
  popd
  pushd $BUILD_DIR/momo/build/macos/webrtc/src/buildtools/third_party/libc++abi/trunk
    echo "WEBRTC_SRC_BUILDTOOLS_THIRD_PARTY_LIBCXXABI_TRUNK=`git rev-parse HEAD`" >> $INSTALL_DIR/webrtc/VERSIONS
  popd
  pushd $BUILD_DIR/momo/build/macos/webrtc/src/buildtools/third_party/libunwind/trunk
    echo "WEBRTC_SRC_BUILDTOOLS_THIRD_PARTY_LIBUNWIND_TRUNK=`git rev-parse HEAD`" >> $INSTALL_DIR/webrtc/VERSIONS
  popd
  pushd $BUILD_DIR/momo/build/macos/webrtc/src/third_party
    echo "WEBRTC_SRC_THIRD_PARTY_COMMIT=`git rev-parse HEAD`" >> $INSTALL_DIR/webrtc/VERSIONS
  popd
  pushd $BUILD_DIR/momo/build/macos/webrtc/src/tools
    echo "WEBRTC_SRC_TOOLS_COMMIT=`git rev-parse HEAD`" >> $INSTALL_DIR/webrtc/VERSIONS
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
