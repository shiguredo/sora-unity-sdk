#!/bin/bash

set -ex

BUILD_DIR="`pwd`/_build"
INSTALL_DIR="`pwd`/_install"

source `pwd`/VERSIONS

mkdir -p $BUILD_DIR
mkdir -p $INSTALL_DIR

# WebRTC のインストール
for name in macos android ios; do
  if [ ! -e $INSTALL_DIR/$name/webrtc/lib/libwebrtc.a ]; then
    # shiguredo-webrtc-build から各環境のバイナリをダウンロードして配置するだけ
    pushd $BUILD_DIR
      rm -rf webrtc.$name.tar.gz
      curl -LO https://github.com/shiguredo-webrtc-build/webrtc-build/releases/download/m${WEBRTC_BUILD_VERSION}/webrtc.$name.tar.gz
    popd

    mkdir -p $INSTALL_DIR/$name
    pushd $INSTALL_DIR/$name
      rm -rf webrtc/
      tar xf $BUILD_DIR/webrtc.$name.tar.gz
    popd
  fi
done

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

# Android 側からのコールバックする関数は消してはいけないので、
# libwebrtc.a の中から消してはいけない関数の一覧を作っておく
if [ ! -e $INSTALL_DIR/android/webrtc.ldflags ]; then
  # readelf を使って libwebrtc.a の関数一覧を列挙して、その中から Java_org_webrtc_ を含む関数を取り出し、
  # -Wl,--undefined=<関数名> に加工する。
  # （-Wl,--undefined はアプリケーションから参照されていなくても関数を削除しないためのフラグ）
  _READELF=$INSTALL_DIR/android-ndk/toolchains/llvm/prebuilt/darwin-x86_64/bin/aarch64-linux-android-readelf
  _LIBWEBRTC_A=$INSTALL_DIR/android/webrtc/lib/arm64-v8a/libwebrtc.a
  $_READELF -Ws $_LIBWEBRTC_A \
    | grep Java_org_webrtc_ \
    | while read a b c d e f g h; do echo -Wl,--undefined=$h; done \
    | sort \
    > $INSTALL_DIR/android/webrtc.ldflags
fi

# 特定バージョンの libcxx, libcxxabi を取得
source $INSTALL_DIR/macos/webrtc/VERSIONS
pushd $INSTALL_DIR
  if [ ! -e libcxx/.git ]; then
    git clone $WEBRTC_SRC_BUILDTOOLS_THIRD_PARTY_LIBCXX_TRUNK_URL
  fi
  pushd libcxx
    git fetch
    git reset --hard $WEBRTC_SRC_BUILDTOOLS_THIRD_PARTY_LIBCXX_TRUNK_COMMIT
  popd

  if [ ! -e libcxxabi/.git ]; then
    git clone $WEBRTC_SRC_BUILDTOOLS_THIRD_PARTY_LIBCXXABI_TRUNK_URL
  fi
  pushd libcxxabi
    git fetch
    git reset --hard $WEBRTC_SRC_BUILDTOOLS_THIRD_PARTY_LIBCXXABI_TRUNK_COMMIT
  popd
popd
