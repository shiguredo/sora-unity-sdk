
$BUILD_DIR = Join-Path (Resolve-Path ".").Path "_build"
$INSTALL_DIR = Join-Path (Resolve-Path ".").Path "_install"

$BOOST_VERSION = "1.71.0"
$JSON_VERSION = "3.6.1"
$WEBRTC_VERSION = "m78.8.0"

mkdir $BUILD_DIR -Force
mkdir $INSTALL_DIR -Force

# Boost

if (!(Test-Path "$INSTALL_DIR\boost\include\boost\version.hpp")) {
  $_BOOST_UNDERSCORE_VERSION = $BOOST_VERSION.Replace(".", "_")
  $_URL = "https://dl.bintray.com/boostorg/release/${BOOST_VERSION}/source/boost_${_BOOST_UNDERSCORE_VERSION}.zip"
  $_FILE = "boost_${_BOOST_UNDERSCORE_VERSION}.zip"
  # ダウンロードと展開
  Push-Location $BUILD_DIR
    if (!(Test-Path $_FILE)) {
      Invoke-WebRequest -Uri $_URL -OutFile $_FILE
    }
    if (!(Test-Path "boost_${_BOOST_UNDERSCORE_VERSION}")) {
      Remove-Item boost_${_BOOST_UNDERSCORE_VERSION} -Force -Recurse
    }
    Expand-Archive -Path $_FILE -DestinationPath .
  Pop-Location
  # ビルドとインストール
  Push-Location $BUILD_DIR\boost_${_BOOST_UNDERSCORE_VERSION}
    & .\bootstrap.bat
    & .\b2.exe install `
        -j8 `
        --prefix=$INSTALL_DIR\boost `
        --with-filesystem `
        --with-date_time `
        --with-regex `
        --layout=system `
        address-model=64 `
        link=static `
        threading=multi `
        variant=release `
        runtime-link=static
  Pop-Location
}

# JSON

if (!(Test-Path "$INSTALL_DIR\json\include\nlohmann\json.hpp")) {
  $_URL = "https://github.com/nlohmann/json/releases/download/v${JSON_VERSION}/include.zip"
  $_FILE = "$BUILD_DIR\json.zip"
  # ダウンロード
  Push-Location $BUILD_DIR
    if (!(Test-Path $_FILE)) {
      Invoke-WebRequest -Uri $_URL -OutFile $_FILE
    }
  Pop-Location
  # 展開(=インストール)
  mkdir "$INSTALL_DIR\json" -Force
  Expand-Archive -Path $_FILE -DestinationPath "$INSTALL_DIR\json"
}

# WebRTC

if (!(Test-Path "$INSTALL_DIR\webrtc\lib\webrtc.lib")) {
  # shiguredo-webrtc-windows のバイナリをダウンロードする
  $_URL = "https://github.com/shiguredo/shiguredo-webrtc-windows/releases/download/$WEBRTC_VERSION/webrtc.zip"
  $_FILE = "$BUILD_DIR\webrtc-$WEBRTC_VERSION.zip"
  Push-Location $BUILD_DIR
    if (!(Test-Path $_FILE)) {
      Invoke-WebRequest -Uri $_URL -OutFile $_FILE
    }
  Pop-Location
  # 展開
  if (Test-Path "$BUILD_DIR\webrtc") {
    Remove-Item $BUILD_DIR\webrtc -Recurse -Force
  }
  mkdir $BUILD_DIR\webrtc -Force
  Expand-Archive -Path $_FILE -DestinationPath "$BUILD_DIR\webrtc"

  # インストール
  if (Test-Path "$INSTALL_DIR\webrtc") {
    Remove-Item $INSTALL_DIR\webrtc -Recurse -Force
  }
  mkdir $INSTALL_DIR\webrtc -Force
  mkdir $INSTALL_DIR\webrtc\lib -Force
  Move-Item $BUILD_DIR\webrtc\release\webrtc.lib $INSTALL_DIR\webrtc\lib
  Move-Item $BUILD_DIR\webrtc\include $INSTALL_DIR\webrtc\include
}