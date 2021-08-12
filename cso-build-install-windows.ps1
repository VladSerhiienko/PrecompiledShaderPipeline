
Set-Alias -Name cmake -Value .\tools\cmake\cmake-3.18.2-win64-x64\bin\cmake.exe
Get-Command cmake | Select-Object -ExpandProperty Definition

cmake -G "Visual Studio 16 2019" -H"." -Bbuild_msbuild2019_win64
cmake --build .\build_msbuild2019_win64\ --config Release