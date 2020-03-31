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

if (!(Test-Path "$INSTALL_DIR\boost\include\boost\version.hpp")) {
  $_BOOST_UNDERSCORE_VERSION = $BOOST_VERSION.Replace(".", "_")
  $_URL = "https://dl.bintray.com/boostorg/release/${BOOST_VERSION}/source/boost_${_BOOST_UNDERSCORE_VERSION}.zip"
  $_FILE = "boost_${_BOOST_UNDERSCORE_VERSION}.zip"
  # ダウンロードと展開
  Push-Location $BUILD_DIR
    if (!(Test-Path $_FILE)) {
      Invoke-WebRequest -Uri $_URL -OutFile $_FILE
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
  # Expand-Archive -Path $_FILE -DestinationPath "$INSTALL_DIR\json"
  Push-Location $INSTALL_DIR\json
    7z x $_FILE
  Pop-Location
}

# WebRTC

if (!(Test-Path "$INSTALL_DIR\webrtc\release\webrtc.lib")) {
  # shiguredo-webrtc-windows のバイナリをダウンロードする
  $_URL = "https://github.com/shiguredo-webrtc-build/webrtc-build/releases/download/m$WEBRTC_BUILD_VERSION/webrtc.windows.zip"
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

# CUDA

if (!(Test-Path "$INSTALL_DIR\cuda_installed")) {
  #$_URL = "http://developer.download.nvidia.com/compute/cuda/10.2/Prod/network_installers/cuda_10.2.89_win10_network.exe"
  #$_FILE = "$BUILD_DIR\cuda_10.2.89_win10_network.exe"
  $_URL = "http://developer.download.nvidia.com/compute/cuda/10.2/Prod/local_installers/cuda_10.2.89_441.22_win10.exe"
  $_FILE = "$BUILD_DIR\cuda_10.2.89_441.22_win10.exe"
  Push-Location $BUILD_DIR
    if (!(Test-Path $_FILE)) {
      Invoke-WebRequest -Uri $_URL -OutFile $_FILE
    }
  Pop-Location
  # サイレントインストール
  # https://docs.nvidia.com/cuda/cuda-installation-guide-microsoft-windows/index.html#download-cuda-software
  #Start-Process "$_FILE" -Wait -ArgumentList "-log $BUILD_DIR\cuda_log.txt -loglevel:6 -s"
  if (Test-Path "$BUILD_DIR\cuda") {
    Remove-Item "$BUILD_DIR\cuda" -Recurse -Force
  }
  mkdir $BUILD_DIR\cuda -Force
  Push-Location $BUILD_DIR\cuda
    7z x $_FILE
    Start-Process setup.exe -Wait -ArgumentList "-log:$BUILD_DIR\cuda\log -loglevel:6 -s nvcc_10.2 cuobjdump_10.2 nvprune_10.2 cupti_10.2 gpu_library_advisor_10.2 memcheck_10.2 nvdisasm_10.2 nvprof_10.2 visual_profiler_10.2 visual_studio_integration_10.2 demo_suite_10.2 documentation_10.2 cublas_10.2 cublas_dev_10.2 cudart_10.2 cufft_10.2 cufft_dev_10.2 curand_10.2 curand_dev_10.2 cusolver_10.2 cusolver_dev_10.2 cusparse_10.2 cusparse_dev_10.2 nvgraph_10.2 nvgraph_dev_10.2 npp_10.2 npp_dev_10.2 nvrtc_10.2 nvrtc_dev_10.2 nvml_dev_10.2 occupancy_calculator_10.2"
  Pop-Location

  Write-Output "---- LOG.setup.exe.log ----"
  Get-Content "$BUILD_DIR\cuda\log\LOG.setup.exe.log"
  if (Test-Path "$BUILD_DIR\cuda\log\LOG.setup.exe.log1") {
    Write-Output "---- LOG.setup.exe.log1 ----"
    Get-Content "$BUILD_DIR\cuda\log\LOG.setup.exe.log1"
  }
  if (Test-Path "$BUILD_DIR\cuda\log\LOG.setup.exe.log2") {
    Write-Output "---- LOG.setup.exe.log2 ----"
    Get-Content "$BUILD_DIR\cuda\log\LOG.setup.exe.log2"
  }
  if (Test-Path "$BUILD_DIR\cuda\log\LOG.RunDll32.EXE.log") {
    Write-Output "---- LOG.RunDll32.EXE.log ----"
    Get-Content "$BUILD_DIR\cuda\log\LOG.RunDll32.EXE.log"
  }
  New-Item -Type File "$INSTALL_DIR\cuda_installed"
}