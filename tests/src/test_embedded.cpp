
#include <PrecompiledShaderPipelineUtils.h>
#include <cso_generated.h>
#include <flatbuffers/flatbuffers.h>
#include <gtest/gtest.h>

#include <Viewer.cso.h>
#include <array>

namespace {

class EmbeddedPrecompiledShaderPipelineTest : public testing::Test {
public:
    EmbeddedPrecompiledShaderPipelineTest() = default;
    ~EmbeddedPrecompiledShaderPipelineTest() = default;

    void SetUp() override {
        pCollection = cso::GetCompiledShaderCollection(get_viewer_cso_buffer_view().data());
        EXPECT_TRUE(pCollection);
    }
    
    void TearDown() override {
        pCollection = nullptr;
    }

protected:
    const cso::CompiledShaderCollection* pCollection = nullptr;
};


TEST_F(EmbeddedPrecompiledShaderPipelineTest, CheckConstantCountUsingUtilityClasses) {
    using namespace cso::utils;
    PrecompiledShaderLibrary library = {pCollection};
    PrecompiledShaderVariant variant = library.FindBestMatch("SceneSkinnedTest.vert", {});
    EXPECT_TRUE(variant.IsValid());
    EXPECT_TRUE(variant.IsCompiled());
    EXPECT_TRUE(variant.IsReflected());

    PrecompiledShaderReflection reflection = variant.Reflection();
    EXPECT_TRUE(reflection.IsValid());
    EXPECT_EQ(reflection.ConstantCount(), 3);
}

} // namespace


#define viewer_cso_paste_implementation
#include <Viewer.cso.h>
