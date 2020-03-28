rm -rf bin
mkdir bin
mkdir bin/macOS
cp ./build_appleclang_xcode/Release/PrecompiledShaderPipeline ./bin/macOS/PrecompiledShaderPipeline
cp ./build_appleclang_xcode/Release/PrecompiledShaderPipelineTests ./bin/macOS/PrecompiledShaderPipelineTests
cp ./build_appleclang_xcode/flatbuffers-prefix/src/flatbuffers-build/Release/flatc ./bin/macOS/flatc
cp ./build_appleclang_xcode/shaderc-prefix/src/shaderc-build/third_party/glslang/StandAlone/Release/glslangValidator ./bin/macOS/glslangValidator
