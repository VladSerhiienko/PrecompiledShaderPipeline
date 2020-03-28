#include <AssetManager.h>

#include <cso_generated.h>

#include <flatbuffers/flatbuffers.h>
#include <gtest/gtest.h>
#include <array>

extern int BuildLibrary(int argc, char** argv);

namespace {
TEST(PrecompiledShaderPipelineTest, Negative) {
    constexpr std::array<const char*, 9> argv = {
        "./PrecompiledShaderPipelineTests",
        "--mode", "build-collection",
        "--assets-folder", "../../assets/**",
        "--cso-json-file", "../../assets/shaders/Viewer.cso.json",
        "--cso-file", "../../assets/shaders/Viewer.cso"};
  
    int buildLibraryResult = BuildLibrary(argv.size(), (char**)argv.data());
    EXPECT_EQ(buildLibraryResult, 0);
    
    using FileReader = apemode::platform::shared::FileReader;
    
    const std::vector<uint8_t> libraryBuffer = FileReader().ReadBinFile("../../assets/shaders/Viewer.cso");
    EXPECT_TRUE(!libraryBuffer.empty());
    
    auto library = cso::GetCompiledShaderCollection(libraryBuffer.data());
    EXPECT_TRUE(library);
    EXPECT_TRUE(library->compiled_shaders() && library->compiled_shaders()->size());
    EXPECT_TRUE(library->reflected_shaders() && library->reflected_shaders()->size());
    
    auto compiledShaderInfos = library->compiled_shader_infos();
    EXPECT_TRUE(compiledShaderInfos);
    
    auto compiledShaderInfoEndIt = compiledShaderInfos->cend();

    auto compiledShaderInfoIt =
    std::find_if(compiledShaderInfos->cbegin(), compiledShaderInfoEndIt, [library](const cso::CompiledShaderInfo* compiledShaderInfo) {
        EXPECT_TRUE(library->strings()->size() > 0);
        EXPECT_TRUE(compiledShaderInfo);
        const uint32_t assetId = compiledShaderInfo->asset_string_index();
        EXPECT_LT(assetId, library->strings()->size());
        const std::string_view asset = library->strings()->Get(assetId)->contents()->string_view();
        EXPECT_FALSE(asset.empty());
        return asset == "shaders/SceneSkinnedTest.vert";
    });

    EXPECT_NE(compiledShaderInfoIt, compiledShaderInfoEndIt);
    
    auto compiledShaderInfo = *compiledShaderInfoIt;
    EXPECT_EQ(compiledShaderInfo->type(), cso::Shader_Vertex);
    EXPECT_LT(compiledShaderInfo->compiled_shader_index(), library->compiled_shaders()->size());
    
    auto compiledShader = library->compiled_shaders()->Get(compiledShaderInfo->compiled_shader_index());
    EXPECT_EQ(compiledShader->type(), cso::IR_SPIRV);
    EXPECT_LT(compiledShader->reflected_shader_index(), library->reflected_shaders()->size());
    
    auto reflectedShader = library->reflected_shaders()->Get(compiledShader->reflected_shader_index());
    EXPECT_TRUE(reflectedShader->constant_indices() && reflectedShader->constant_indices()->size());
    EXPECT_EQ(reflectedShader->constant_indices()->size(), 2);
    
    /*
    + constants (2):
    ├ - kBoneCount
    │   └── Int (size=4)
    └ - kConstScalar
        └── Double (size=8)
    + stage inputs (6):
    ├ - inPosition (set=?, binding=?, location=0, is_active=yes)
    │   └── Float3 (size=12)
    ├ - inBoneWeights (set=?, binding=?, location=4, is_active=yes)
    │   └── Float4 (size=16)
    ├ - inBoneIndices (set=?, binding=?, location=5, is_active=yes)
    │   └── Float4 (size=16)
    ├ - inTexcoords (set=?, binding=?, location=3, is_active=yes)
    │   └── Float2 (size=8)
    ├ - inNormal (set=?, binding=?, location=1, is_active=yes)
    │   └── Float3 (size=12)
    └ - inTangent (set=?, binding=?, location=2, is_active=yes)
        └── Float4 (size=16)
    + stage outputs (6):
    ├ - outWorldPosition (set=?, binding=?, location=0, is_active=yes)
    │   └── Float3 (size=12)
    ├ - outViewDirection (set=?, binding=?, location=4, is_active=yes)
    │   └── Float3 (size=12)
    ├ - outTexcoords (set=?, binding=?, location=5, is_active=yes)
    │   └── Float2 (size=8)
    ├ - outWorldNormal (set=?, binding=?, location=1, is_active=yes)
    │   └── Float3 (size=12)
    ├ - outWorldTangent (set=?, binding=?, location=2, is_active=yes)
    │   └── Float3 (size=12)
    └ - outWorldBitangent (set=?, binding=?, location=3, is_active=yes)
        └── Float3 (size=12)
    + uniform buffers (2):
    ├ - <unnamed> (set=0, binding=0, location=?, active_ranges=[(0:192)])
    │   └── CameraUBO (size=256)
    │       ├── ViewMatrix : Float4x4 (offset=0, size=64)
    │       ├── ProjMatrix : Float4x4 (offset=64, size=64)
    │       ├── InvViewMatrix : Float4x4 (offset=128, size=64)
    │       └── InvProjMatrix : Float4x4 (offset=192, size=64)
    └ - <unnamed> (set=1, binding=0, location=?, active_ranges=[(0:9344),(9360:16),(9392:32)])
        └── ObjectUBO (size=9460)
            ├── WorldMatrix : Float4x4 (offset=0, size=64)
            ├── BoneOffsetMatrices : Float4x4[72|?] (offset=64, size=4608)
            ├── NormalMatrix : Float4x4 (offset=4672, size=64)
            ├── BoneNormalMatrices : Float4x4[72|?] (offset=4736, size=4608)
            ├── OtherPositionOffset : Float3 (offset=9344, effective_size=12, total_size=16, padding=4)
            ├── PositionOffset : Float4 (offset=9360, size=16)
            ├── OtherPositionScale : Float (offset=9376, effective_size=4, total_size=16, padding=12)
            ├── PositionScale : Float4 (offset=9392, size=16)
            ├── TexcoordOffsetScale : Float4 (offset=9408, size=16)
            ├── OtherSampleStruct : SampleStruct (offset=9424, size=32)
            │   ├── x : Float (offset=0, size=4)
            │   ├── y : Float (offset=4, size=4)
            │   ├── z : Float (offset=8, size=4)
            │   ├── w : Float (offset=12, size=4)
            │   └── vxyzw : Float4 (offset=16, size=16)
            └── OtherTexcoordOffsetScale : Float (offset=9456, size=4)
    */
    
    
}
}
