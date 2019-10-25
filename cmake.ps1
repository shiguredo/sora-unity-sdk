$INSTALL_DIR = (Join-Path (Resolve-Path ".").Path "_install").Replace("\", "/")
$MODULE_PATH = (Join-Path (Resolve-Path ".").Path "cmake").Replace("\", "/")

mkdir build -Force
Push-Location build
  cmake .. -G "Visual Studio 16 2019" `
    -DCMAKE_BUILD_TYPE=Release `
    -DWEBRTC_ROOT_DIR="$INSTALL_DIR/webrtc" `
    -DJSON_ROOT_DIR"=$INSTALL_DIR/json" `
    -DCMAKE_MODULE_PATH="$MODULE_PATH" `
    -DCMAKE_PREFIX_PATH="$INSTALL_DIR/boost"
  cmake --build . --config Release
Pop-Location