rm -rf bin/macOS
mkdir bin/macOS
cp ./build_appleclang_xcode/Release/PrecompiledShaderPipeline ./bin/macOS/PrecompiledShaderPipeline
cp ./build_appleclang_xcode/Release/PrecompiledShaderPipelineTests ./bin/macOS/PrecompiledShaderPipelineTests
cp ./build_appleclang_xcode/flatbuffers-prefix/src/flatbuffers-build/Release/flatc ./bin/macOS/flatc
cp ./build_appleclang_xcode/shaderc-prefix/src/shaderc-build/third_party/glslang/StandAlone/Release/glslangValidator ./bin/macOS/glslangValidator

rm -rf ./include/flatbuffers
mkdir -p ./include/flatbuffers
cp -r ./dependencies/flatbuffers/include ./include/flatbuffers
cp -r ./dependencies/flatbuffers/LICENSE.txt ./include/flatbuffers/LICENSE.txt
