$ErrorActionPreference = 'Stop'

$WEBRTC_VERSION_FILE = Join-Path (Resolve-Path ".").Path "_install" | Join-Path -ChildPath "webrtc" | Join-Path -ChildPath "VERSIONS"
Get-Content $WEBRTC_VERSION_FILE | Foreach-Object{
  $var = $_.Split('=')
  New-Variable -Name $var[0] -Value $var[1]
}
$SORA_VERSION_FILE = Join-Path (Resolve-Path ".").Path "VERSIONS"
Get-Content $SORA_VERSION_FILE | Foreach-Object{
  $var = $_.Split('=')
  New-Variable -Name $var[0] -Value $var[1]
}

$SORA_UNITY_SDK_COMMIT = "$(git rev-parse HEAD)"

$INSTALL_DIR = (Join-Path (Resolve-Path ".").Path "_install").Replace("\", "/")
$MODULE_PATH = (Join-Path (Resolve-Path ".").Path "cmake").Replace("\", "/")

mkdir build -Force
Push-Location build
  cmake .. -G "Visual Studio 16 2019" `
    -DSORA_UNITY_SDK_VERSION="$SORA_UNITY_SDK_VERSION" `
    -DSORA_UNITY_SDK_COMMIT="$SORA_UNITY_SDK_COMMIT" `
    -DWEBRTC_VERSION="$WEBRTC_VERSION" `
    -DWEBRTC_READABLE_VERSION="$WEBRTC_READABLE_VERSION" `
    -DWEBRTC_SRC_COMMIT="$WEBRTC_SRC_COMMIT" `
    -DWEBRTC_ROOT_DIR="$INSTALL_DIR/webrtc" `
    -DJSON_ROOT_DIR="$INSTALL_DIR/json" `
    -DCMAKE_MODULE_PATH="$MODULE_PATH" `
    -DCMAKE_PREFIX_PATH="$INSTALL_DIR/boost"
  cmake --build . --config Release
Pop-Location