cmake -G Xcode -H. -Bbuild_appleclang_xcode
xcodebuild -project ./build_appleclang_xcode/PrecompiledShaderPipeline.xcodeproj -configuration Release
