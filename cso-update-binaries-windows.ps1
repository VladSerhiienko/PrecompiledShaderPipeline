Remove-Item "bin/Windows" -Recurse -Force -ErrorAction Ignore
New-Item -ItemType Directory -Path "bin/Windows"
Copy-Item ./build_msbuild2019_win64/Release/* ./bin/Windows -Recurse -Force -Filter *.exe
Copy-Item ./build_msbuild2019_win64/Release/* ./bin/Windows -Recurse -Force -Filter *.dll
Copy-Item ./build_msbuild2019_win64/flatbuffers-prefix/src/flatbuffers-build/Release/* ./bin/Windows -Recurse -Force -Filter *.exe
Copy-Item ./build_msbuild2019_win64/flatbuffers-prefix/src/flatbuffers-build/Release/* ./bin/Windows -Recurse -Force -Filter *.dll
Copy-Item ./build_msbuild2019_win64/shaderc-prefix/src/shaderc-build/third_party/glslang/StandAlone/Release/* ./bin/Windows -Recurse -Force -Filter *.exe
Copy-Item ./build_msbuild2019_win64/shaderc-prefix/src/shaderc-build/third_party/glslang/StandAlone/Release/* ./bin/Windows -Recurse -Force -Filter *.dll