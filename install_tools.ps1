$ErrorActionPreference = 'Stop'

$BUILD_DIR = Join-Path (Resolve-Path ".").Path "_build"
$INSTALL_DIR = Join-Path (Resolve-Path ".").Path "_install"

$SORA_VERSION_FILE = Join-Path (Resolve-Path ".").Path "VERSIONS"
Get-Content $SORA_VERSION_FILE | Foreach-Object{
  if (!$_) {
    continue
  }
  $var = $_.Split('=')
  New-Variable -Name $var[0] -Value $var[1]
}

mkdir $BUILD_DIR -Force
mkdir $INSTALL_DIR -Force

# Boost

$BOOST_VERSION_FILE = "$INSTALL_DIR\boost.version"
$BOOST_CHANGED = $FALSE
if (!(Test-Path $BOOST_VERSION_FILE) -Or ("$BOOST_VERSION" -ne (Get-Content $BOOST_VERSION_FILE))) {
  $BOOST_CHANGED = $TRUE
}

if ($BOOST_CHANGED -Or !(Test-Path "$INSTALL_DIR\boost\include\boost\version.hpp")) {
  $_BOOST_UNDERSCORE_VERSION = $BOOST_VERSION.Replace(".", "_")
  $_URL = "https://boostorg.jfrog.io/artifactory/main/release/${BOOST_VERSION}/source/boost_${_BOOST_UNDERSCORE_VERSION}.zip"
  $_FILE = "boost_${_BOOST_UNDERSCORE_VERSION}.zip"
  # ダウンロードと展開
  Push-Location $BUILD_DIR
    if (!(Test-Path $_FILE)) {
      # curl に擬態しないとアーカイブではなく HTML コンテンツが降ってきてしまう
      Invoke-WebRequest -Headers @{"User-Agent"="curl/7.55.1"} -Uri $_URL -OutFile $_FILE
    }
    if (Test-Path "boost_${_BOOST_UNDERSCORE_VERSION}") {
      Remove-Item boost_${_BOOST_UNDERSCORE_VERSION} -Force -Recurse
    }
    # Expand-Archive -Path $_FILE -DestinationPath .
    7z x $_FILE
  Pop-Location
  # ビルドとインストール
  Push-Location $BUILD_DIR\boost_${_BOOST_UNDERSCORE_VERSION}
    & .\bootstrap.bat
    & .\b2.exe install `
        cxxstd=17 `
        -j8 `
        --prefix=$INSTALL_DIR\boost `
        --with-filesystem `
        --with-container `
        --with-json `
        --layout=system `
        address-model=64 `
        link=static `
        threading=multi `
        variant=release `
        runtime-link=static
  Pop-Location
}
Set-Content "$BOOST_VERSION_FILE" -Value "$BOOST_VERSION"

# WebRTC

$WEBRTC_VERSION_FILE = "$INSTALL_DIR\webrtc.version"
$WEBRTC_CHANGED = $FALSE
if (!(Test-Path $WEBRTC_VERSION_FILE) -Or ("$WEBRTC_BUILD_VERSION" -ne (Get-Content $WEBRTC_VERSION_FILE))) {
  $WEBRTC_CHANGED = $TRUE
}

if ($WEBRTC_CHANGED -Or !(Test-Path "$INSTALL_DIR\webrtc\release\webrtc.lib")) {
  # shiguredo-webrtc-windows のバイナリをダウンロードする
  $_URL = "https://github.com/shiguredo-webrtc-build/webrtc-build/releases/download/m$WEBRTC_BUILD_VERSION/webrtc.windows_x86_64.zip"
  $_FILE = "$BUILD_DIR\webrtc-m$WEBRTC_BUILD_VERSION.zip"
  Push-Location $BUILD_DIR
    if (!(Test-Path $_FILE)) {
      Invoke-WebRequest -Uri $_URL -OutFile $_FILE
    }
  Pop-Location
  # 展開
  if (Test-Path "$BUILD_DIR\webrtc") {
    Remove-Item $BUILD_DIR\webrtc -Recurse -Force
  }
  # Expand-Archive -Path $_FILE -DestinationPath "$BUILD_DIR\webrtc"
  Push-Location $BUILD_DIR
    7z x $_FILE
  Pop-Location

  # インストール
  if (Test-Path "$INSTALL_DIR\webrtc") {
    Remove-Item $INSTALL_DIR\webrtc -Recurse -Force
  }
  Move-Item $BUILD_DIR\webrtc $INSTALL_DIR\webrtc
}
Set-Content "$WEBRTC_VERSION_FILE" -Value "$WEBRTC_BUILD_VERSION"

# CUDA

$CUDA_VERSION_FILE = "$INSTALL_DIR\cuda.version"
$CUDA_CHANGED = $FALSE
if (!(Test-Path $CUDA_VERSION_FILE) -Or ("$CUDA_VERSION" -ne (Get-Content $CUDA_VERSION_FILE))) {
  $CUDA_CHANGED = $TRUE
}

if ($CUDA_CHANGED -Or !(Test-Path "$INSTALL_DIR\cuda\nvcc")) {
  if ("$CUDA_VERSION" -eq "10.2") {
    $_URL = "http://developer.download.nvidia.com/compute/cuda/10.2/Prod/local_installers/cuda_10.2.89_441.22_win10.exe"
    $_FILE = "$BUILD_DIR\cuda_10.2.89_441.22_win10.exe"
  } else {
    # バージョンが増えたらこの分岐を増やしていく
    throw "CUDA-$CUDA_VERSION URL not specified"
  }

  Push-Location $BUILD_DIR
    if (!(Test-Path $_FILE)) {
      Invoke-WebRequest -Uri $_URL -OutFile $_FILE
    }
  Pop-Location
  if (Test-Path "$BUILD_DIR\cuda") {
    Remove-Item "$BUILD_DIR\cuda" -Recurse -Force
  }
  mkdir $BUILD_DIR\cuda -Force
  Push-Location $BUILD_DIR\cuda
    # サイレントインストールとかせずに、単に展開だけして nvcc を利用する
    7z x $_FILE
    if (Test-Path "$INSTALL_DIR\cuda") {
      Remove-Item $INSTALL_DIR\cuda -Recurse -Force
    }
    mkdir $INSTALL_DIR\cuda
    Move-Item nvcc $INSTALL_DIR\cuda\nvcc
  Pop-Location
}
Set-Content "$CUDA_VERSION_FILE" -Value "$CUDA_VERSION"


# protobuf
$PROTOBUF_VERSION_FILE = "$INSTALL_DIR\protobuf.version"
$PROTOBUF_CHANGED = $FALSE
if (!(Test-Path $PROTOBUF_VERSION_FILE) -Or ("$PROTOBUF_VERSION" -ne (Get-Content $PROTOBUF_VERSION_FILE))) {
  $PROTOBUF_CHANGED = $TRUE
}

if ($PROTOBUF_CHANGED -Or !(Test-Path "$INSTALL_DIR\protobuf\bin\protoc.exe")) {
  $_URL = "https://github.com/protocolbuffers/protobuf/releases/download/v$PROTOBUF_VERSION/protoc-$PROTOBUF_VERSION-win64.zip"
  $_FILE = "$BUILD_DIR/protoc-$PROTOBUF_VERSION-win64.zip"

  Push-Location $BUILD_DIR
    if (!(Test-Path $_FILE)) {
      Invoke-WebRequest -Uri $_URL -OutFile $_FILE
    }
  Pop-Location
  if (Test-Path "$BUILD_DIR\protoc") {
    Remove-Item "$BUILD_DIR\protoc" -Recurse -Force
  }
  mkdir $BUILD_DIR\protoc -Force
  Push-Location $BUILD_DIR\protoc
    7z x $_FILE
  Pop-Location
  if (Test-Path "$INSTALL_DIR\protoc") {
    Remove-Item $INSTALL_DIR\protoc -Recurse -Force
  }
  Move-Item $BUILD_DIR\protoc $INSTALL_DIR\protoc
}
Set-Content "$PROTOBUF_VERSION_FILE" -Value "$PROTOBUF_VERSION"

# protoc-gen-jsonif
$PROTOC_GEN_JSONIF_VERSION_FILE = "$INSTALL_DIR\protoc-gen-jsonif.version"
$PROTOC_GEN_JSONIF_CHANGED = $FALSE
if (!(Test-Path $PROTOC_GEN_JSONIF_VERSION_FILE) -Or ("$PROTOC_GEN_JSONIF_VERSION" -ne (Get-Content $PROTOC_GEN_JSONIF_VERSION_FILE))) {
  $PROTOC_GEN_JSONIF_CHANGED = $TRUE
}

if ($PROTOC_GEN_JSONIF_CHANGED -Or !(Test-Path "$INSTALL_DIR\protoc-gen-jsonif\bin\protoc-gen-jsonif-cpp.exe")) {
  $_URL = "https://github.com/melpon/protoc-gen-jsonif/releases/download/$PROTOC_GEN_JSONIF_VERSION/protoc-gen-jsonif.zip"
  $_FILE = "$BUILD_DIR/protoc-gen-jsonif.zip"

  Push-Location $BUILD_DIR
    if (!(Test-Path $_FILE)) {
      Invoke-WebRequest -Uri $_URL -OutFile $_FILE
    }
  Pop-Location
  if (Test-Path "$BUILD_DIR\protoc-gen-jsonif") {
    Remove-Item "$BUILD_DIR\protoc-gen-jsonif" -Recurse -Force
  }
  mkdir $BUILD_DIR -Force
  Push-Location $BUILD_DIR
    7z x $_FILE
    if (Test-Path "$INSTALL_DIR\protoc-gen-jsonif") {
      Remove-Item $INSTALL_DIR\protoc-gen-jsonif -Recurse -Force
    }
    Move-Item protoc-gen-jsonif $INSTALL_DIR\protoc-gen-jsonif
    Copy-Item -Recurse $INSTALL_DIR\protoc-gen-jsonif\windows\amd64 $INSTALL_DIR\protoc-gen-jsonif\bin
  Pop-Location
}
Set-Content "$PROTOC_GEN_JSONIF_VERSION_FILE" -Value "$PROTOC_GEN_JSONIF_VERSION"