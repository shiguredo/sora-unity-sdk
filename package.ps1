$ErrorActionPreference = 'Stop'

. .\cmake.ps1
if (Test-Path "SoraUnitySdk.zip") {
    Remove-Item SoraUnitySdk.zip
}
if (Test-Path "SoraUnitySdk\") {
    Remove-Item "SoraUnitySdk\" -Force -Recurse
}
New-Item "SoraUnitySdk\Plugins\SoraUnitySdk\windows\x86_64" -ItemType Directory -ErrorAction 0
New-Item "SoraUnitySdk\SoraUnitySdk\" -ItemType Directory -ErrorAction 0
Copy-Item "Sora\Sora.cs" "SoraUnitySdk\SoraUnitySdk\"
Copy-Item ".\build\Release\SoraUnitySdk.dll" "SoraUnitySdk\Plugins\SoraUnitySdk\windows\x86_64"
Compress-Archive -Path ".\SoraUnitySdk" -DestinationPath SoraUnitySdk.zip