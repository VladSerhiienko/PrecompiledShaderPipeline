cmake -G "Visual Studio 16 2019" -A x64 -T"LLVM-vs2014" -H"." -Bbuild_llvm_msbuild2019_win64
cmake --build .\build_llvm_msbuild2019_win64\ --config Release