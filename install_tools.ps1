
$BUILD_DIR = Join-Path (Resolve-Path ".").Path "_build"
$INSTALL_DIR = Join-Path (Resolve-Path ".").Path "_install"

$BOOST_VERSION = "1.71.0"
$JSON_VERSION = "3.6.1"

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
    Remove-Item boost_${_BOOST_UNDERSCORE_VERSION} -Force -Recurse
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
  # shiguredo-webrtc-windows を使ってライブラリを用意する
  Push-Location $BUILD_DIR
    Remove-Item shiguredo-webrtc-windows -Force
    git clone https://github.com/shiguredo/shiguredo-webrtc-windows.git
  Pop-Location
  # ビルド
  Push-Location $BUILD_DIR\shiguredo-webrtc-windows
    & .\build.bat
  Pop-Location
  # インストール
  mkdir $INSTALL_DIR\webrtc -Force
  mkdir $INSTALL_DIR\webrtc\lib -Force
  Remove-Item $INSTALL_DIR\webrtc\include -Force
  Push-Location $BUILD_DIR\shiguredo-webrtc-windows
    Move-Item release\webrtc.lib $INSTALL_DIR\webrtc\lib
    Move-Item include $INSTALL_DIR\webrtc\include
  Pop-Location
}