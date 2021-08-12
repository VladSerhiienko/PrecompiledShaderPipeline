
$cmake_url = "https://github.com/Kitware/CMake/releases/download/v3.18.2/cmake-3.18.2-win64-x64.zip"

if (-not (Test-Path "tools")) {
    mkdir tools
}

Set-Location tools

if (-not (Test-Path "cmake")) {
    Invoke-WebRequest $cmake_url -OutFile "cmake.zip"
    Expand-Archive "cmake.zip" -DestinationPath "cmake"
}

Set-Location ..
