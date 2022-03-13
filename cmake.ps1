$ErrorActionPreference = 'Stop'

$WEBRTC_VERSION_FILE = Join-Path (Resolve-Path ".").Path "_install" | Join-Path -ChildPath "x86_64" | Join-Path -ChildPath "webrtc" | Join-Path -ChildPath "VERSIONS"
Get-Content $WEBRTC_VERSION_FILE | Foreach-Object{
  if (!$_) {
    continue
  }
  $var = $_.Split('=')
  New-Variable -Name $var[0] -Value $var[1] -Force
}
$SORA_VERSION_FILE = Join-Path (Resolve-Path ".").Path "VERSIONS"
Get-Content $SORA_VERSION_FILE | Foreach-Object{
  if (!$_) {
    continue
  }
  $var = $_.Split('=')
  New-Variable -Name $var[0] -Value $var[1] -Force
}

$SORA_UNITY_SDK_COMMIT = "$(git rev-parse HEAD)"

$INSTALL_DIR = (Join-Path (Resolve-Path ".").Path "_install").Replace("\", "/")
$MODULE_PATH = (Join-Path (Resolve-Path ".").Path "cmake").Replace("\", "/")

mkdir build\x86_64 -Force
Push-Location build\x86_64
  cmake ..\.. -G "Visual Studio 16 2019" `
    -DSORA_UNITY_SDK_PACKAGE=windows_x86_64 `
    -DSORA_UNITY_SDK_VERSION="$SORA_UNITY_SDK_VERSION" `
    -DSORA_UNITY_SDK_COMMIT="$SORA_UNITY_SDK_COMMIT" `
    -DWEBRTC_BUILD_VERSION="$WEBRTC_BUILD_VERSION" `
    -DWEBRTC_READABLE_VERSION="$WEBRTC_READABLE_VERSION" `
    -DWEBRTC_SRC_COMMIT="$WEBRTC_SRC_COMMIT" `
    -DWEBRTC_ROOT_DIR="$INSTALL_DIR/x86_64/webrtc" `
    -DJSON_ROOT_DIR="$INSTALL_DIR/json" `
    -DCMAKE_MODULE_PATH="$MODULE_PATH" `
    -DCMAKE_PREFIX_PATH="$INSTALL_DIR/x86_64/boost"
  cmake --build . -j8 --config Release
Pop-Location

mkdir build\hololens2 -Force
Push-Location build\hololens2
  cmake ..\.. -G "Visual Studio 16 2019" `
    -A ARM64 `
    -DSORA_UNITY_SDK_PACKAGE=windows_hololens2 `
    -DSORA_UNITY_SDK_VERSION="$SORA_UNITY_SDK_VERSION" `
    -DSORA_UNITY_SDK_COMMIT="$SORA_UNITY_SDK_COMMIT" `
    -DWEBRTC_BUILD_VERSION="$WEBRTC_BUILD_VERSION" `
    -DWEBRTC_READABLE_VERSION="$WEBRTC_READABLE_VERSION" `
    -DWEBRTC_SRC_COMMIT="$WEBRTC_SRC_COMMIT" `
    -DWEBRTC_ROOT_DIR="$INSTALL_DIR/hololens2/webrtc" `
    -DJSON_ROOT_DIR="$INSTALL_DIR/json" `
    -DCMAKE_MODULE_PATH="$MODULE_PATH" `
    -DCMAKE_PREFIX_PATH="$INSTALL_DIR/hololens2/boost"
  cmake --build . -j8 --config Release
Pop-Location
