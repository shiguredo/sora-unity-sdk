#!/bin/bash

set -ex

BUILD_DIR="`pwd`/_build"
INSTALL_DIR="`pwd`/_install"

source `pwd`/VERSIONS

mkdir -p $BUILD_DIR
mkdir -p $INSTALL_DIR

# WebRTC のインストール
WEBRTC_VERSION_FILE="$INSTALL_DIR/webrtc.version"
WEBRTC_CHANGED=0
if [ ! -e $WEBRTC_VERSION_FILE -o "$WEBRTC_BUILD_VERSION" != "`cat $WEBRTC_VERSION_FILE`" ]; then
  WEBRTC_CHANGED=1
fi

for name in macos android ios; do
  if [ $WEBRTC_CHANGED -eq 1 -o ! -e $INSTALL_DIR/$name/webrtc ]; then
    pkgname=$name
    if [ "$name" == "macos" ]; then
      pkgname=macos_x86_64
    fi

    # shiguredo-webrtc-build から各環境のバイナリをダウンロードして配置するだけ
    pushd $BUILD_DIR
      rm -rf webrtc.$name.tar.gz
      curl -LO https://github.com/shiguredo-webrtc-build/webrtc-build/releases/download/m${WEBRTC_BUILD_VERSION}/webrtc.$pkgname.tar.gz
    popd

    mkdir -p $INSTALL_DIR/$name
    pushd $INSTALL_DIR/$name
      rm -rf webrtc/
      tar xf $BUILD_DIR/webrtc.$pkgname.tar.gz
    popd

    rm -rf $INSTALL_DIR/libcxx/
    rm -rf $INSTALL_DIR/libcxxabi/

    rm -f $INSTALL_DIR/android/webrtc.ldflags
  fi
done
echo $WEBRTC_BUILD_VERSION > $WEBRTC_VERSION_FILE

# Android NDK のインストール
ANDROID_NDK_VERSION_FILE="$INSTALL_DIR/android_ndk.version"
ANDROID_NDK_CHANGED=0
if [ ! -e $ANDROID_NDK_VERSION_FILE -o "$ANDROID_NDK_VERSION" != "`cat $ANDROID_NDK_VERSION_FILE`" ]; then
  ANDROID_NDK_CHANGED=1
fi

if [ $ANDROID_NDK_CHANGED -eq 1 -o ! -e $INSTALL_DIR/android-ndk ]; then
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
  rm -f $INSTALL_DIR/android/webrtc.ldflags
fi
echo $ANDROID_NDK_VERSION > $ANDROID_NDK_VERSION_FILE

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
if [ ! -e $INSTALL_DIR/libcxx/.git ]; then
  git clone $WEBRTC_SRC_BUILDTOOLS_THIRD_PARTY_LIBCXX_TRUNK_URL $INSTALL_DIR/libcxx
fi
pushd $INSTALL_DIR/libcxx
  git fetch
  git reset --hard $WEBRTC_SRC_BUILDTOOLS_THIRD_PARTY_LIBCXX_TRUNK_COMMIT
popd

if [ ! -e $INSTALL_DIR/libcxxabi/.git ]; then
  git clone $WEBRTC_SRC_BUILDTOOLS_THIRD_PARTY_LIBCXXABI_TRUNK_URL $INSTALL_DIR/libcxxabi
fi
pushd $INSTALL_DIR/libcxxabi
  git fetch
  git reset --hard $WEBRTC_SRC_BUILDTOOLS_THIRD_PARTY_LIBCXXABI_TRUNK_COMMIT
popd

# __config_site のために特定バージョンの buildtools を取得する
if [ ! -e $INSTALL_DIR/buildtools/.git ]; then
  git clone $WEBRTC_SRC_BUILDTOOLS_URL $INSTALL_DIR/buildtools
fi
pushd $INSTALL_DIR/buildtools
  git fetch
  git reset --hard $WEBRTC_SRC_BUILDTOOLS_COMMIT
  cp third_party/libc++/__config_site $INSTALL_DIR/libcxx/include/
popd

# Boost
BOOST_VERSION_FILE="$INSTALL_DIR/boost.version"
BOOST_CHANGED=0
if [ ! -e $BOOST_VERSION_FILE -o "$BOOST_VERSION" != "`cat $BOOST_VERSION_FILE`" ]; then
  BOOST_CHANGED=1
fi

for name in macos android ios; do
  if [ $BOOST_CHANGED -eq 1 -o ! -e $INSTALL_DIR/$name/boost/include/boost/version.hpp ]; then
    _VERSION_UNDERSCORE=${BOOST_VERSION//./_}
    _URL=https://boostorg.jfrog.io/artifactory/main/release/${BOOST_VERSION}/source/boost_${_VERSION_UNDERSCORE}.tar.gz
    _FILE=$BUILD_DIR/boost_${_VERSION_UNDERSCORE}.tar.gz
    if [ ! -e $_FILE ]; then
      echo "file(DOWNLOAD $_URL $_FILE)" > $BUILD_DIR/tmp.cmake
      cmake -P $BUILD_DIR/tmp.cmake
      rm $BUILD_DIR/tmp.cmake
    fi
    mkdir -p $BUILD_DIR/$name
    pushd $BUILD_DIR/$name
      rm -rf boost_${_VERSION_UNDERSCORE}
      cmake -E tar xf $_FILE

      pushd boost_${_VERSION_UNDERSCORE}
        ./bootstrap.sh
        rm -rf $INSTALL_DIR/$name/boost

        case "$name" in
          "android" )
            echo " \
              using clang \
              : android \
              : $INSTALL_DIR/android-ndk/toolchains/llvm/prebuilt/darwin-x86_64/bin/aarch64-linux-android${ANDROID_NATIVE_API_LEVEL}-clang++ \
              : <archiver>\"$INSTALL_DIR/android-ndk/toolchains/llvm/prebuilt/darwin-x86_64/bin/aarch64-linux-android-ar\" \
                <ranlib>\"$INSTALL_DIR/android-ndk/toolchains/llvm/prebuilt/darwin-x86_64/bin/aarch64-linux-android-ranlib\" ; \
              " > user-config.jam
            TARGET_OS=android
            BOOST_CXXFLAGS=" \
              -D_LIBCPP_ABI_UNSTABLE \
              -isystem $INSTALL_DIR/libcxx/include \
              -nostdinc++ \
              --sysroot=$INSTALL_DIR/android-ndk/toolchains/llvm/prebuilt/darwin-x86_64/sysroot \
            "
            BOOST_FLAGS=" \
              architecture=arm \
              --user-config=user-config.jam \
            "
            ;;
          "macos" )
            BOOST_CXXFLAGS=""
            BOOST_FLAGS=""
            TARGET_OS=darwin
            ;;
          "ios" )
            BOOST_CXXFLAGS=" \
              -fvisibility=default \
              -mios-version-min=10.0 \
              -fvisibility=hidden \
              -fvisibility-inlines-hidden \
            "
            BOOST_FLAGS=""
            TARGET_OS=iphone
            ;;
        esac

        B2_INSTALL_FLAGS=" \
            cxxstd=17 \
            -j8 \
            --with-filesystem \
            --with-container \
            --with-json \
            visibility=global \
            toolset=clang \
            target-os=$TARGET_OS \
            address-model=64 \
            link=static \
            threading=multi \
            variant=release \
            --ignore-site-config \
            $BOOST_FLAGS \
        "
        if [ "$name" = "ios" ]; then
          # シミュレータとデバイス用に生成して lipo でくっつける
          rm -rf $INSTALL_DIR/$name-simulator/boost
          rm -rf $INSTALL_DIR/$name-device/boost
          CLANGPP=`xcodebuild -find clang++`
          echo " \
            using clang \
            : iphone \
            : $CLANGPP -arch x86_64 \
            : <striper> <root>`xcrun --sdk iphonesimulator --show-sdk-path` \
            : <architecture>x64 <target-os>iphone <address-model>64 \
            ; \
            " > user-config.jam
          ./b2 install \
            $B2_INSTALL_FLAGS \
            cflags="-arch x86_64 -isysroot `xcrun --sdk iphonesimulator --show-sdk-path` $BOOST_CXXFLAGS" \
            cxxflags="-arch x86_64 -isysroot `xcrun --sdk iphonesimulator --show-sdk-path` $BOOST_CXXFLAGS" \
            --build-dir=./$name-simulator \
            --prefix=$INSTALL_DIR/$name-simulator/boost

          echo " \
            using clang \
            : iphone \
            : $CLANGPP -arch arm64 -fembed-bitcode \
            : <striper> <root>`xcrun --sdk iphoneos --show-sdk-path` \
            : <architecture>arm <target-os>iphone <address-model>64 \
            ; \
            " > user-config.jam
          ./b2 install \
            $B2_INSTALL_FLAGS \
            cflags="-arch arm64 -isysroot `xcrun --sdk iphoneos --show-sdk-path` $BOOST_CXXFLAGS" \
            cxxflags="-arch arm64 -isysroot `xcrun --sdk iphoneos --show-sdk-path` $BOOST_CXXFLAGS" \
            --build-dir=./$name-device \
            --prefix=$INSTALL_DIR/$name-device/boost \
            architecture=arm
          mkdir -p $INSTALL_DIR/$name/boost/lib
          for filename in `cd $INSTALL_DIR/$name-device/boost/lib && ls -1 *.a`; do
            lipo -create -output $INSTALL_DIR/$name/boost/lib/$filename \
              $INSTALL_DIR/$name-simulator/boost/lib/$filename \
              $INSTALL_DIR/$name-device/boost/lib/$filename
          done
          mv $INSTALL_DIR/$name-device/boost/include $INSTALL_DIR/$name/boost/include
          rm -rf $INSTALL_DIR/$name-simulator
          rm -rf $INSTALL_DIR/$name-device
        else
          ./b2 install \
            $B2_INSTALL_FLAGS \
            cxxflags="$BOOST_CXXFLAGS" \
            --prefix=$INSTALL_DIR/$name/boost
        fi
      popd
    popd
  fi
done
echo $BOOST_VERSION > $BOOST_VERSION_FILE

# protobuf
PROTOBUF_VERSION_FILE="$INSTALL_DIR/protobuf.version"
PROTOBUF_CHANGED=0
if [ ! -e $PROTOBUF_VERSION_FILE -o "$PROTOBUF_VERSION" != "`cat $PROTOBUF_VERSION_FILE`" ]; then
  PROTOBUF_CHANGED=1
fi

if [ $PROTOBUF_CHANGED -eq 1 -o ! -e $INSTALL_DIR/protobuf/bin/protoc ]; then
  if [ "`uname`" == "Darwin" ]; then
    _URL=https://github.com/protocolbuffers/protobuf/releases/download/v$PROTOBUF_VERSION/protoc-$PROTOBUF_VERSION-osx-x86_64.zip
    _FILE=$BUILD_DIR/protoc-$PROTOBUF_VERSION-osx-x86_64.zip
  else
    _URL=https://github.com/protocolbuffers/protobuf/releases/download/v$PROTOBUF_VERSION/protoc-$PROTOBUF_VERSION-linux-x86_64.zip
    _FILE=$BUILD_DIR/protoc-$PROTOBUF_VERSION-linux-x86_64.zip
  fi
  mkdir -p $BUILD_DIR
  if [ ! -e $_FILE ]; then
    echo "file(DOWNLOAD $_URL $_FILE)" > $BUILD_DIR/tmp.cmake
    cmake -P $BUILD_DIR/tmp.cmake
    rm $BUILD_DIR/tmp.cmake
  fi
  rm -rf $BUILD_DIR/protoc
  rm -rf $INSTALL_DIR/protoc
  mkdir $BUILD_DIR/protoc
  pushd $BUILD_DIR/protoc
    unzip $_FILE
  popd
  mv $BUILD_DIR/protoc $INSTALL_DIR/protoc
fi
echo $PROTOBUF_VERSION > $PROTOBUF_VERSION_FILE

# protoc-gen-jsonif
PROTOC_GEN_JSONIF_VERSION_FILE="$INSTALL_DIR/protoc-gen-jsonif.version"
PROTOC_GEN_JSONIF_CHANGED=0
if [ ! -e $PROTOC_GEN_JSONIF_VERSION_FILE -o "$PROTOC_GEN_JSONIF_VERSION" != "`cat $PROTOC_GEN_JSONIF_VERSION_FILE`" ]; then
  PROTOC_GEN_JSONIF_CHANGED=1
fi

if [ $PROTOC_GEN_JSONIF_CHANGED -eq 1 -o ! -e $INSTALL_DIR/protoc-gen-jsonif/bin/protoc ]; then
  _URL=https://github.com/melpon/protoc-gen-jsonif/releases/download/$PROTOC_GEN_JSONIF_VERSION/protoc-gen-jsonif.tar.gz
  _FILE=$BUILD_DIR/protoc-gen-jsonif.tar.gz
  mkdir -p $BUILD_DIR
  if [ ! -e $_FILE ]; then
    echo "file(DOWNLOAD $_URL $_FILE)" > $BUILD_DIR/tmp.cmake
    cmake -P $BUILD_DIR/tmp.cmake
    rm $BUILD_DIR/tmp.cmake
  fi
  rm -rf $BUILD_DIR/protoc-gen-jsonif
  rm -rf $INSTALL_DIR/protoc-gen-jsonif
  pushd $BUILD_DIR
    tar -xf $_FILE
    mv protoc-gen-jsonif $INSTALL_DIR/protoc-gen-jsonif
    if [ "`uname`" == "Darwin" ]; then
      cp -r $INSTALL_DIR/protoc-gen-jsonif/darwin/amd64 $INSTALL_DIR/protoc-gen-jsonif/bin
    else
      cp -r $INSTALL_DIR/protoc-gen-jsonif/linux/amd64 $INSTALL_DIR/protoc-gen-jsonif/bin
    fi
    chmod +x $INSTALL_DIR/protoc-gen-jsonif/bin/*
  popd
fi
echo $PROTOC_GEN_JSONIF_VERSION > $PROTOC_GEN_JSONIF_VERSION_FILE
