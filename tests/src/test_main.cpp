
#include <PrecompiledShaderPipelineUtils.h>
#include <cso_generated.h>
#include <flatbuffers/flatbuffers.h>
#include <gtest/gtest.h>

#include <array>
#include <fstream>
#include <iterator>
#include <cassert>

extern int BuildLibrary(int argc, char** argv);

namespace {

class PrecompiledShaderPipelineTest : public testing::Test {
public:
    PrecompiledShaderPipelineTest() = default;
    ~PrecompiledShaderPipelineTest() = default;

    void SetUp() override {
        constexpr std::array<const char*, 5> argv = {"./PrecompiledShaderPipelineTests",
                                                     "--mode=build-collection",
                                                     "--input-file=../../tests/assets/shaders/Viewer.cso.json",
                                                     "--output-file=../../tests/assets/shaders/Viewer.cso",
                                                     "--add-path=../../tests/assets/shaders"};

        int buildLibraryResult = BuildLibrary(argv.size(), (char**)argv.data());
        assert(buildLibraryResult, 0);

        std::ifstream viewerCSO("../../tests/assets/shaders/Viewer.cso", std::ios::binary);
        collectionBuffer =
            std::vector<int8_t>(std::istreambuf_iterator<char>(viewerCSO), std::istreambuf_iterator<char>());
        assert(!collectionBuffer.empty());

        pCollection = cso::GetCompiledShaderCollection(collectionBuffer.data());
        assert(pCollection);
    }

    void TearDown() override {
        pCollection = nullptr;
        decltype(collectionBuffer)().swap(collectionBuffer);
    }

protected:
    std::vector<int8_t> collectionBuffer = {};
    const cso::CompiledShaderCollection* pCollection = nullptr;
};

//
// Why use utility classes
//

TEST_F(PrecompiledShaderPipelineTest, PainfulCheckConstantCountUsingRawFlatbuffers) {
    auto library = pCollection;
    EXPECT_TRUE(library);
    EXPECT_TRUE(library->compiled_shaders() && library->compiled_shaders()->size());
    EXPECT_TRUE(library->reflected_shaders() && library->reflected_shaders()->size());

    auto compiledShaderInfos = library->compiled_shader_infos();
    EXPECT_TRUE(compiledShaderInfos);

    auto compiledShaderInfoEndIt = compiledShaderInfos->cend();
    auto compiledShaderInfoIt = std::find_if(compiledShaderInfos->cbegin(),
                                             compiledShaderInfoEndIt,
                                             [library](const cso::CompiledShaderInfo* compiledShaderInfo) {
                                                 EXPECT_TRUE(library->strings()->size() > 0);
                                                 EXPECT_TRUE(compiledShaderInfo);
                                                 const uint32_t assetId = compiledShaderInfo->asset_string_index();
                                                 EXPECT_LT(assetId, library->strings()->size());
                                                 const std::string_view asset =
                                                     library->strings()->Get(assetId)->contents()->string_view();
                                                 EXPECT_FALSE(asset.empty());
                                                 return asset == "SceneSkinnedTest.vert";
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
    EXPECT_GE(reflectedShader->constant_indices()->size(), 2);
}

TEST_F(PrecompiledShaderPipelineTest, CheckConstantCountUsingUtilityClasses) {
    using namespace cso::utils;
    PrecompiledShaderLibrary library = {pCollection};
    PrecompiledShaderVariant variant = library.FindBestMatch("SceneSkinnedTest.vert", {});
    EXPECT_TRUE(variant.IsValid());
    EXPECT_TRUE(variant.IsCompiled());
    EXPECT_TRUE(variant.IsReflected());

    PrecompiledShaderReflection reflection = variant.Reflection();
    EXPECT_TRUE(reflection.IsValid());
    EXPECT_GE(reflection.ConstantCount(), 2);
}

//
// Actual coverage
//

} // namespace
