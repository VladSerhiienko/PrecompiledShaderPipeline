cmake -G "Visual Studio 16 2019" -H"." -Bbuild_msbuild2019_win64
cmake --build .\build_msbuild2019_win64\ --config Debug
cmake --build .\build_msbuild2019_win64\ --config Release