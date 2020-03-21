#include "ShaderCompiler.Vulkan.h"

#include <apemode/platform/AppState.h>

#include <spirv_glsl.hpp>
#include <spirv_msl.hpp>
#include <spirv_reflect.hpp>

using namespace apemode::shp;

namespace {

constexpr std::string_view ToString(spirv_cross::SPIRType::BaseType bt) {
    switch (bt) { // clang-format off
        case spirv_cross::SPIRType::Unknown: return "Unknown";
        case spirv_cross::SPIRType::Void: return "Void";
        case spirv_cross::SPIRType::Boolean: return "Boolean";
        case spirv_cross::SPIRType::SByte: return "SByte";
        case spirv_cross::SPIRType::UByte: return "UByte";
        case spirv_cross::SPIRType::Short: return "Short";
        case spirv_cross::SPIRType::UShort: return "UShort";
        case spirv_cross::SPIRType::Int: return "Int";
        case spirv_cross::SPIRType::UInt: return "UInt";
        case spirv_cross::SPIRType::Int64: return "Int64";
        case spirv_cross::SPIRType::UInt64: return "UInt64";
        case spirv_cross::SPIRType::AtomicCounter: return "AtomicCounter";
        case spirv_cross::SPIRType::Half: return "Half";
        case spirv_cross::SPIRType::Float: return "Float";
        case spirv_cross::SPIRType::Double: return "Double";
        case spirv_cross::SPIRType::Struct: return "Struct";
        case spirv_cross::SPIRType::Image: return "Image";
        case spirv_cross::SPIRType::SampledImage: return "SampledImage";
        case spirv_cross::SPIRType::Sampler: return "Sampler";
        case spirv_cross::SPIRType::AccelerationStructureNV: return "AccelerationStructureNV";
        case spirv_cross::SPIRType::ControlPointArray: return "ControlPointArray";
        case spirv_cross::SPIRType::Char: return "Char";
        default: assert(false); return "<Error>";
    }; // clang-format on
}
constexpr ReflectedPrimitiveType Reflect(spirv_cross::SPIRType::BaseType bt) {
    switch (bt) { // clang-format off
        case spirv_cross::SPIRType::Struct: return eReflectedPrimitiveType_Struct;
        case spirv_cross::SPIRType::Boolean: return eReflectedPrimitiveType_Bool;
        case spirv_cross::SPIRType::Char: return eReflectedPrimitiveType_Char;
        case spirv_cross::SPIRType::SByte: return eReflectedPrimitiveType_Char;
        case spirv_cross::SPIRType::UByte: return eReflectedPrimitiveType_UChar;
        case spirv_cross::SPIRType::Short: return eReflectedPrimitiveType_Short;
        case spirv_cross::SPIRType::UShort: return eReflectedPrimitiveType_UShort;
        case spirv_cross::SPIRType::Int: return eReflectedPrimitiveType_Int;
        case spirv_cross::SPIRType::UInt: return eReflectedPrimitiveType_UInt;
        case spirv_cross::SPIRType::Int64: return eReflectedPrimitiveType_Long;
        case spirv_cross::SPIRType::UInt64: return eReflectedPrimitiveType_ULong;
        case spirv_cross::SPIRType::Half: return eReflectedPrimitiveType_Half;
        case spirv_cross::SPIRType::Float: return eReflectedPrimitiveType_Float;
        case spirv_cross::SPIRType::Double: return eReflectedPrimitiveType_Double;
        case spirv_cross::SPIRType::Image: return eReflectedPrimitiveType_Image;
        case spirv_cross::SPIRType::SampledImage: return eReflectedPrimitiveType_SampledImage;
        case spirv_cross::SPIRType::Sampler: return eReflectedPrimitiveType_Sampler;
        default: assert(false); return eReflectedPrimitiveTypeError;
    }; // clang-format on
}
constexpr size_t PrimitiveTypeSize(ReflectedPrimitiveType primitiveType) {
    switch (primitiveType) { // clang-format off
        case eReflectedPrimitiveType_Bool: return 1;
        case eReflectedPrimitiveType_Char: return 1;
        case eReflectedPrimitiveType_UChar: return 1;
        case eReflectedPrimitiveType_Short: return 2;
        case eReflectedPrimitiveType_UShort: return 2;
        case eReflectedPrimitiveType_Int: return 4;
        case eReflectedPrimitiveType_UInt: return 4;
        case eReflectedPrimitiveType_Long: return 8;
        case eReflectedPrimitiveType_ULong: return 8;
        case eReflectedPrimitiveType_Half: return 2;
        case eReflectedPrimitiveType_Float: return 4;
        case eReflectedPrimitiveType_Double: return 8;
        default: return 0;
    }; // clang-format on
}

void PrintReflection(const ReflectedType& reflectedType,
                     const std::string& prefix,
                     bool isLastItem,
                     const std::string& memberName = "",
                     uint32_t offset = -1,
                     uint32_t total_size = -1) {
    std::string outputString = prefix;
    outputString += isLastItem ? "└── " : "├── ";

    if (!memberName.empty()) { outputString += memberName + " : "; }

    outputString += reflectedType.name;

    if (offset != -1 && total_size != -1) {
        outputString += " (offset=" + std::to_string(offset);
        outputString += ", size=" + std::to_string(total_size);

        assert(total_size >= reflectedType.effective_byte_size && "Caught size miscalculation.");
        if (uint32_t padding = total_size - reflectedType.effective_byte_size) {
            outputString += ", padding=" + std::to_string(padding);
        }
        outputString += ")";
    } else if (reflectedType.effective_byte_size) {
        outputString += " (size=" + std::to_string(reflectedType.effective_byte_size);
        outputString += ")";
    }

    apemode::LogInfo("{}", outputString);

    if (!reflectedType.reflected_member_types.empty()) {
        uint32_t memberCount = reflectedType.reflected_member_types.size();
        for (uint32_t i = 0; i < memberCount; ++i) {
            PrintReflection(reflectedType.reflected_member_types[i]->type,
                            prefix + (!isLastItem ? "│   " : "    "),
                            (i + 1) == memberCount,
                            reflectedType.reflected_member_types[i]->name,
                            reflectedType.reflected_member_types[i]->offset,
                            reflectedType.reflected_member_types[i]->occupied_size);
        }
    }
}
void PrintReflection(const ReflectedResource& reflectedResource, const std::string& prefix, bool isLastItem) {
    std::string outputString = prefix;
    outputString += isLastItem ? "└ - " : "├ - ";
    outputString += reflectedResource.name.empty() ? "<unnamed>" : reflectedResource.name;
    outputString += " (set=" + ((reflectedResource.descriptor_set == -1) ? "?" : std::to_string(reflectedResource.descriptor_set));
    outputString += ", binding=" + ((reflectedResource.binding == -1) ? "?" : std::to_string(reflectedResource.binding));
    outputString += ", location=" + ((reflectedResource.location == -1) ? "?" : std::to_string(reflectedResource.location));
    outputString += ")";

    apemode::LogInfo("{}", outputString);
    PrintReflection(reflectedResource.reflected_type, prefix + (!isLastItem ? "│   " : "    "), true);
}
void PrintReflection(const ReflectedConstant& reflectedConstant, const std::string& prefix, bool last) {
    std::string outputString = prefix;
    outputString += last ? "└ - " : "├ - ";
    outputString += reflectedConstant.name;

    apemode::LogInfo("{}", outputString);
    PrintReflection(reflectedConstant.reflected_type, prefix + (!last ? "│   " : "    "), true);
}
void PrintReflection(const std::vector<ReflectedConstant>& reflectedConstants) {
    if (reflectedConstants.empty()) { return; }

    apemode::LogInfo("+ constants ({}):", reflectedConstants.size());

    uint32_t count = reflectedConstants.size();
    for (uint32_t i = 0; i < count; ++i) { PrintReflection(reflectedConstants[i], "", (i + 1) == count); }
}
void PrintReflection(const std::vector<ReflectedResource>& reflectedResources, const std::string& resourceKind) {
    if (reflectedResources.empty()) { return; }

    apemode::LogInfo("+ {} ({}):", resourceKind, reflectedResources.size());

    uint32_t count = reflectedResources.size();
    for (uint32_t i = 0; i < count; ++i) { PrintReflection(reflectedResources[i], "", (i + 1) == count); }
}

void PrintReflection(const ReflectedShader& reflectedShader) {
    PrintReflection(reflectedShader.reflected_constants);
    PrintReflection(reflectedShader.reflected_stage_inputs, "stage inputs");
    PrintReflection(reflectedShader.reflected_stage_outputs, "stage outputs");
    PrintReflection(reflectedShader.reflected_uniform_buffers, "uniform buffers");
    PrintReflection(reflectedShader.reflected_push_constant_buffers, "push constants");
    PrintReflection(reflectedShader.reflected_images, "images");
    PrintReflection(reflectedShader.reflected_samplers, "samplers");
    PrintReflection(reflectedShader.reflected_sampled_images, "sampled images");
    PrintReflection(reflectedShader.reflected_storage_images, "storage images");
    PrintReflection(reflectedShader.reflected_storage_buffers, "storage buffers");
    PrintReflection(reflectedShader.reflected_subpass_inputs, "subpass inputs");
}
} // namespace

class CompiledShader : public ICompiledShader {
public:
    apemode::vector<uint32_t> Dwords = {};
    apemode::string8 PreprocessedSrc = {};
    apemode::string8 AssemblySrc = {};
    apemode::string8 CompiledGLSL = {};
    apemode::string8 CompiledMSL = {};
    spirv_cross::CompilerGLSL CompilerGLSL;
    spirv_cross::CompilerMSL CompilerMSL;
    spirv_cross::Compiler& Reflection;
    apemode::shp::ReflectedShader Reflected = {};

    CompiledShader(apemode::vector<uint32_t>&& dwords,
                   apemode::string8&& preprocessedSrc,
                   apemode::string8&& assemblySrc)
        : Dwords(std::move(dwords))
        , PreprocessedSrc(std::move(preprocessedSrc))
        , AssemblySrc(std::move(assemblySrc))
        , CompilerGLSL(Dwords.data(), Dwords.size())
        , CompilerMSL(Dwords.data(), Dwords.size())
        , Reflection(CompilerGLSL) { // Dwords.data(), Dwords.size()) {

        spirv_cross::CompilerGLSL::Options glslOptions = {};
        glslOptions.vulkan_semantics = true;
        CompilerGLSL.set_common_options(glslOptions);

        CompiledGLSL = apemode::string8(CompilerGLSL.compile().c_str());
        CompiledMSL = apemode::string8(CompilerMSL.compile().c_str());
        PopulateReflection();
    }

    ~CompiledShader() {}

    void ReflectMemberType(const spirv_cross::SPIRType& type,
                           ReflectedType& reflectedType,
                           uint32_t member_type_index) {
        reflectedType.reflected_member_types[member_type_index] = new ReflectedStructMember();
        auto& reflectedMemberType = *reflectedType.reflected_member_types[member_type_index];

        const spirv_cross::SPIRType& memberType = Reflection.get_type(type.member_types[member_type_index]);
        reflectedMemberType.name = Reflection.get_member_name(type.self, member_type_index);
        reflectedMemberType.offset = Reflection.type_struct_member_offset(type, member_type_index);
        reflectedMemberType.effective_size = Reflection.get_declared_struct_member_size(type, member_type_index);
        reflectedMemberType.occupied_size = reflectedMemberType.effective_size;

        reflectedMemberType.type = ReflectType(memberType, reflectedMemberType.effective_size);

        if (!memberType.array.empty()) {
            const uint32_t arrayStride = Reflection.type_struct_member_array_stride(type, member_type_index);
            assert(arrayStride == reflectedMemberType.type.array_byte_stride && "Caught paddings.");
            reflectedMemberType.type.array_byte_stride = arrayStride;
        }

        if (memberType.columns > 1) {
            uint32_t matrixStride = Reflection.type_struct_member_matrix_stride(type, member_type_index);
            assert(matrixStride == reflectedMemberType.type.element_matrix_byte_stride && "Caught paddings.");
            reflectedMemberType.type.element_matrix_byte_stride = matrixStride;
        }
    }

    ReflectedType ReflectType(const spirv_cross::SPIRType& type, uint32_t memberEffectiveSize = 0) {
        ReflectedType reflectedType = {};

        reflectedType.name = Reflection.get_name(type.self);

        reflectedType.array_length = 1;
        reflectedType.is_array_length_static = false;
        reflectedType.element_vector_length = std::max<uint32_t>(1, type.vecsize);
        reflectedType.element_column_count = std::max<uint32_t>(1, type.columns);
        reflectedType.element_primitive_type = Reflect(type.basetype);

        if (reflectedType.element_primitive_type == eReflectedPrimitiveType_Struct) {
            reflectedType.element_byte_size = Reflection.get_declared_struct_size(type);
        } else {
            reflectedType.element_byte_size = PrimitiveTypeSize(reflectedType.element_primitive_type) *
                                              reflectedType.element_vector_length *
                                              reflectedType.element_column_count;
        }

        if (!type.array.empty()) {
            reflectedType.is_array_length_static = type.array_size_literal[0];
            if (reflectedType.is_array_length_static) {
                reflectedType.array_length = type.array[0];
                assert(reflectedType.array_length == (memberEffectiveSize / reflectedType.element_byte_size) &&
                       "Caught array size mismatch.");
            } else {
                reflectedType.array_length = memberEffectiveSize / reflectedType.element_byte_size;
            }
        }

        reflectedType.element_matrix_byte_stride = reflectedType.element_byte_size / reflectedType.element_column_count;
        reflectedType.array_byte_stride = reflectedType.element_byte_size;
        reflectedType.effective_byte_size = reflectedType.array_length * reflectedType.array_byte_stride;
        reflectedType.occupied_byte_size = reflectedType.effective_byte_size;

        if (reflectedType.element_primitive_type == eReflectedPrimitiveType_Struct) {
            const size_t member_count = type.member_types.size();
            reflectedType.reflected_member_types.resize(member_count);
            for (size_t i = 0; i < member_count; i++) { ReflectMemberType(type, reflectedType, i); }

            for (size_t i = 0; i < (member_count - 1); i++) {
                const uint32_t nextOffset = reflectedType.reflected_member_types[i + 1]->offset;
                const uint32_t thisOffset = reflectedType.reflected_member_types[i]->offset;
                reflectedType.reflected_member_types[i]->occupied_size = nextOffset - thisOffset;
            }

            reflectedType.occupied_byte_size = reflectedType.reflected_member_types.back()->offset;
            reflectedType.occupied_byte_size += reflectedType.reflected_member_types.back()->type.effective_byte_size;
        }

        if (reflectedType.name.empty()) {
            reflectedType.name = ToString(type.basetype);

            if (reflectedType.element_vector_length > 1 && reflectedType.element_column_count > 1) {
                reflectedType.name += std::to_string(reflectedType.element_vector_length);
                reflectedType.name += "x";
                reflectedType.name += std::to_string(reflectedType.element_column_count);
            } else if (reflectedType.element_vector_length > 1 || reflectedType.element_column_count > 1) {
                const auto length = std::max(reflectedType.element_vector_length, reflectedType.element_column_count);
                reflectedType.name += std::to_string(length);
            }

            if (reflectedType.array_length > 1) {
                reflectedType.name += "[";
                reflectedType.name += std::to_string(reflectedType.array_length);
                reflectedType.name += reflectedType.is_array_length_static ? "|!]" : "|?]";
            }
        }

        return reflectedType;
    }

    uint32_t ReflectDecoration(spirv_cross::ID resourceId, spv::Decoration decoration, uint32_t missingValue = -1) {
        if (!Reflection.has_decoration(resourceId, decoration)) { return missingValue; }
        return Reflection.get_decoration(resourceId, decoration);
    }

    void ReflectResource(const spirv_cross::Resource& resource, ReflectedResource& reflectedResource) {
        const spirv_cross::SPIRType& type = Reflection.get_type(resource.base_type_id);
        reflectedResource.reflected_type = ReflectType(type);
        reflectedResource.name = Reflection.get_name(resource.id);
        reflectedResource.descriptor_set = ReflectDecoration(resource.id, spv::DecorationDescriptorSet);
        reflectedResource.binding = ReflectDecoration(resource.id, spv::DecorationBinding);
        reflectedResource.location = ReflectDecoration(resource.id, spv::DecorationLocation);
    }

    void ReflectResourceVector(const spirv_cross::SmallVector<spirv_cross::Resource>& resources,
                               std::vector<ReflectedResource>& reflectedResources) {
        reflectedResources.resize(resources.size());
        for (size_t i = 0; i < resources.size(); ++i) {
            const spirv_cross::Resource& resource = resources[i];
            auto& reflectedResource = reflectedResources[i];
            ReflectResource(resource, reflectedResource);
        }
    }

    void ReflectConstant(const spirv_cross::SpecializationConstant& constant, ReflectedConstant& reflectedConstant) {
        const spirv_cross::SPIRConstant& spirConstant = Reflection.get_constant(constant.id);
        const spirv_cross::SPIRType& type = Reflection.get_type(spirConstant.constant_type);
        reflectedConstant.name = Reflection.get_name(constant.id);
        reflectedConstant.reflected_type = ReflectType(type);
        reflectedConstant.constant_id = constant.constant_id;
        reflectedConstant.is_specialization = spirConstant.specialization;
        reflectedConstant.is_used_as_array_length = spirConstant.is_used_as_array_length;
        reflectedConstant.is_used_as_lut = spirConstant.is_used_as_lut;
    }

    void ReflectConstantVector(const spirv_cross::SmallVector<spirv_cross::SpecializationConstant>& constants,
                               std::vector<ReflectedConstant>& reflectedConstants) {
        reflectedConstants.resize(constants.size());
        for (size_t i = 0; i < constants.size(); ++i) {
            const spirv_cross::SpecializationConstant& constant = constants[i];
            auto& reflectedConstant = reflectedConstants[i];
            ReflectConstant(constant, reflectedConstant);
        }
    }

    void PopulateReflection() {
        const auto& constants = Reflection.get_specialization_constants();
        ReflectConstantVector(constants, Reflected.reflected_constants);

        const spirv_cross::ShaderResources shaderResources = Reflection.get_shader_resources();
        ReflectResourceVector(shaderResources.uniform_buffers, Reflected.reflected_uniform_buffers);
        ReflectResourceVector(shaderResources.push_constant_buffers, Reflected.reflected_push_constant_buffers);
        ReflectResourceVector(shaderResources.stage_inputs, Reflected.reflected_stage_inputs);
        ReflectResourceVector(shaderResources.stage_outputs, Reflected.reflected_stage_outputs);
        ReflectResourceVector(shaderResources.sampled_images, Reflected.reflected_sampled_images);
        ReflectResourceVector(shaderResources.separate_images, Reflected.reflected_images);
        ReflectResourceVector(shaderResources.separate_samplers, Reflected.reflected_samplers);
        ReflectResourceVector(shaderResources.storage_images, Reflected.reflected_storage_images);
        ReflectResourceVector(shaderResources.storage_buffers, Reflected.reflected_storage_buffers);

        PrintReflection(Reflected);
    }

    // clang-format off
    std::string_view GetPreprocessedSrc() const override { std::string_view sv{PreprocessedSrc.c_str(), PreprocessedSrc.size()}; return sv; }
    std::string_view GetAssemblySrc() const override { std::string_view sv{AssemblySrc.c_str(), AssemblySrc.size()}; return sv; }
    std::string_view GetCompiledGLSL() const override { std::string_view sv{CompiledGLSL.c_str(), CompiledGLSL.size()}; return sv; }
    std::string_view GetCompiledMSL() const override { std::string_view sv{CompiledMSL.c_str(), CompiledMSL.size()}; return sv; }
    const ReflectedShader& GetReflection() const override { return Reflected; };
    // clang-format on

    const uint8_t* GetBytePtr() const override { return reinterpret_cast<const uint8_t*>(Dwords.data()); }
    size_t GetByteCount() const override { return Dwords.size() << 2; }
};

class Includer : public shaderc::CompileOptions::IncluderInterface {
public:
    struct UserData {
        std::string Content;
        std::string Path;
    };

    IShaderCompiler::IShaderFileReader& FileReader;
    IShaderCompiler::IIncludedFileSet* pIncludedFiles;

    Includer(IShaderCompiler::IShaderFileReader& fileReader, IShaderCompiler::IIncludedFileSet* pIncludedFiles)
        : FileReader(fileReader), pIncludedFiles(pIncludedFiles) {}

    // Handles shaderc_include_resolver_fn callbacks.
    shaderc_include_result* GetInclude(const char* pszRequestedSource,
                                       shaderc_include_type eShaderIncludeType,
                                       const char* pszRequestingSource,
                                       size_t includeDepth) {
        apemode_memory_allocation_scope;

        auto userData = apemode::make_unique<UserData>();
        if (userData && pIncludedFiles &&
            FileReader.ReadShaderTxtFile(pszRequestedSource, userData->Path, userData->Content)) {
            pIncludedFiles->InsertIncludedFile(userData->Path);

            auto includeResult = apemode::make_unique<shaderc_include_result>();
            includeResult->content = userData->Content.data();
            includeResult->content_length = userData->Content.size();
            includeResult->source_name = userData->Path.data();
            includeResult->source_name_length = userData->Path.size();
            includeResult->user_data = userData.release();
            return includeResult.release();
        }

        return nullptr;
    }

    // Handles shaderc_include_result_release_fn callbacks.
    void ReleaseInclude(shaderc_include_result* data) {
        apemode_memory_allocation_scope;
        apemode_delete(((UserData*&)data->user_data));
        apemode_delete(data);
    }
};

namespace {
/* Wrapper for shaderc lib */
class ShaderCompiler : public apemode::shp::IShaderCompiler {
public:
public:
    ShaderCompiler();
    virtual ~ShaderCompiler();

    /* @note No files, only ready to compile shader sources */

    apemode::unique_ptr<ICompiledShader> Compile(const std::string& ShaderName,
                                                 const std::string& ShaderCode,
                                                 const IMacroDefinitionCollection* pMacros,
                                                 EShaderType eShaderKind,
                                                 EShaderOptimizationType eShaderOptimization) const override;

    /* @note Compiling from source files */

    IShaderFileReader* GetShaderFileReader() override;
    IShaderFeedbackWriter* GetShaderFeedbackWriter() override;
    void SetShaderFileReader(IShaderFileReader* pShaderFileReader) override;
    void SetShaderFeedbackWriter(IShaderFeedbackWriter* pShaderFeedbackWriter) override;

    apemode::unique_ptr<ICompiledShader> Compile(const std::string& FilePath,
                                                 const IMacroDefinitionCollection* pMacros,
                                                 EShaderType eShaderKind,
                                                 EShaderOptimizationType eShaderOptimization,
                                                 IIncludedFileSet* pOutIncludedFiles) const override;

private:
    shaderc::Compiler Compiler;
    IShaderFileReader* pShaderFileReader = nullptr;
    IShaderFeedbackWriter* pShaderFeedbackWriter = nullptr;
};
} // namespace

ShaderCompiler::ShaderCompiler() {}

ShaderCompiler::~ShaderCompiler() {}

ShaderCompiler::IShaderFileReader* ShaderCompiler::GetShaderFileReader() { return pShaderFileReader; }

ShaderCompiler::IShaderFeedbackWriter* ShaderCompiler::GetShaderFeedbackWriter() { return pShaderFeedbackWriter; }

void ShaderCompiler::SetShaderFileReader(IShaderFileReader* pInShaderFileReader) {
    pShaderFileReader = pInShaderFileReader;
}

void ShaderCompiler::SetShaderFeedbackWriter(IShaderFeedbackWriter* pInShaderFeedbackWriter) {
    pShaderFeedbackWriter = pInShaderFeedbackWriter;
}

static apemode::unique_ptr<apemode::shp::ICompiledShader> InternalCompile(
    const std::string& shaderName,
    const std::string& shaderContent,
    const IShaderCompiler::IMacroDefinitionCollection* pMacros,
    const IShaderCompiler::EShaderType eShaderKind,
    shaderc::CompileOptions& options,
    const bool bAssembly,
    const shaderc::Compiler* pCompiler,
    ShaderCompiler::IShaderFeedbackWriter* pShaderFeedbackWriter) {
    using namespace apemode::shp;
    apemode_memory_allocation_scope;

    if (nullptr == pCompiler) { return nullptr; }

    shaderc::PreprocessedSourceCompilationResult preprocessedSourceCompilationResult =
        pCompiler->PreprocessGlsl(shaderContent, (shaderc_shader_kind)eShaderKind, shaderName.c_str(), options);

    if (shaderc_compilation_status_success != preprocessedSourceCompilationResult.GetCompilationStatus()) {
        if (nullptr != pShaderFeedbackWriter) {
            pShaderFeedbackWriter->WriteFeedback(
                ShaderCompiler::IShaderFeedbackWriter::eFeedbackType_CompilationStage_Preprocessed |
                    preprocessedSourceCompilationResult.GetCompilationStatus(),
                shaderName,
                pMacros,
                preprocessedSourceCompilationResult.GetErrorMessage().data(),
                preprocessedSourceCompilationResult.GetErrorMessage().data() +
                    preprocessedSourceCompilationResult.GetErrorMessage().size());
        }

        assert(false);
        return nullptr;
    }

    if (nullptr != pShaderFeedbackWriter) {
        // apemode::LogInfo("ShaderCompiler: Preprocessed GLSL.");

        pShaderFeedbackWriter->WriteFeedback(ShaderCompiler::IShaderFeedbackWriter::eFeedback_PreprocessingSucceeded,
                                             shaderName,
                                             pMacros,
                                             preprocessedSourceCompilationResult.cbegin(),
                                             preprocessedSourceCompilationResult.cend());
    }

    shaderc::AssemblyCompilationResult assemblyCompilationResult = pCompiler->CompileGlslToSpvAssembly(
        preprocessedSourceCompilationResult.begin(), (shaderc_shader_kind)eShaderKind, shaderName.c_str(), options);

    if (shaderc_compilation_status_success != assemblyCompilationResult.GetCompilationStatus()) {
        apemode::LogError("ShaderCompiler: Failed to compile processed GLSL to SPV assembly: {}.",
                          assemblyCompilationResult.GetErrorMessage().c_str());

        if (nullptr != pShaderFeedbackWriter) {
            pShaderFeedbackWriter->WriteFeedback(
                ShaderCompiler::IShaderFeedbackWriter::eFeedbackType_CompilationStage_Assembly |
                    assemblyCompilationResult.GetCompilationStatus(),
                shaderName,
                pMacros,
                assemblyCompilationResult.GetErrorMessage().data(),
                assemblyCompilationResult.GetErrorMessage().data() +
                    assemblyCompilationResult.GetErrorMessage().size());
        }

        assert(false);
        return nullptr;
    }

    if (nullptr != pShaderFeedbackWriter) {
        // apemode::LogInfo("ShaderCompiler: Compiled GLSL to SPV assembly.");

        pShaderFeedbackWriter->WriteFeedback(ShaderCompiler::IShaderFeedbackWriter::eFeedback_AssemblySucceeded,
                                             shaderName,
                                             pMacros,
                                             assemblyCompilationResult.cbegin(),
                                             assemblyCompilationResult.cend());
    }

    shaderc::SpvCompilationResult spvCompilationResult = pCompiler->CompileGlslToSpv(
        preprocessedSourceCompilationResult.begin(), (shaderc_shader_kind)eShaderKind, shaderName.c_str(), options);

    if (shaderc_compilation_status_success != spvCompilationResult.GetCompilationStatus()) {
        if (nullptr != pShaderFeedbackWriter) {
            // apemode::LogInfo("ShaderCompiler: Failed to compile processed GLSL to SPV: {}.",
            //                  spvCompilationResult.GetErrorMessage().c_str());

            pShaderFeedbackWriter->WriteFeedback(
                ShaderCompiler::IShaderFeedbackWriter::eFeedbackType_CompilationStage_Spv |
                    spvCompilationResult.GetCompilationStatus(),
                shaderName,
                pMacros,
                spvCompilationResult.GetErrorMessage().data(),
                spvCompilationResult.GetErrorMessage().data() + spvCompilationResult.GetErrorMessage().size());
        }

        assert(false);
        return nullptr;
    }

    if (nullptr != pShaderFeedbackWriter) {
        // apemode::LogInfo("ShaderCompiler: Compiled GLSL to SPV.");

        pShaderFeedbackWriter->WriteFeedback(ShaderCompiler::IShaderFeedbackWriter::eFeedback_SpvSucceeded,
                                             shaderName,
                                             pMacros,
                                             spvCompilationResult.cbegin(),
                                             spvCompilationResult.cend());
    }

    apemode::vector<uint32_t> dwords(spvCompilationResult.cbegin(), spvCompilationResult.cend());

    apemode::string8 preprocessedSrc(preprocessedSourceCompilationResult.cbegin(),
                                     preprocessedSourceCompilationResult.cend());

    apemode::string8 assemblySrc(assemblyCompilationResult.cbegin(), assemblyCompilationResult.cend());

    ICompiledShader* pCompiledShader =
        apemode_new CompiledShader(std::move(dwords), std::move(preprocessedSrc), std::move(assemblySrc));

    return apemode::unique_ptr<ICompiledShader>(pCompiledShader);
}

apemode::unique_ptr<apemode::shp::ICompiledShader> ShaderCompiler::Compile(
    const std::string& shaderName,
    const std::string& shaderContent,
    const IMacroDefinitionCollection* pMacros,
    const EShaderType eShaderKind,
    const EShaderOptimizationType eShaderOptimization) const {
    apemode_memory_allocation_scope;

    shaderc::CompileOptions options;
    options.SetSourceLanguage(shaderc_source_language_glsl);
    options.SetOptimizationLevel(shaderc_optimization_level(eShaderOptimization));
    options.SetTargetEnvironment(shaderc_target_env_vulkan, 0);

    return InternalCompile(shaderName,
                           shaderContent,
                           pMacros,
                           eShaderKind,
                           options,
                           true,
                           /* impl */ &Compiler,
                           /* impl */ pShaderFeedbackWriter);
}

apemode::unique_ptr<apemode::shp::ICompiledShader> ShaderCompiler::Compile(
    const std::string& InFilePath,
    const IMacroDefinitionCollection* pMacros,
    const EShaderType eShaderKind,
    const EShaderOptimizationType eShaderOptimization,
    IIncludedFileSet* pOutIncludedFiles) const {
    apemode_memory_allocation_scope;

    shaderc::CompileOptions options;
    options.SetSourceLanguage(shaderc_source_language_glsl);
    options.SetOptimizationLevel(shaderc_optimization_level(eShaderOptimization));
    options.SetTargetEnvironment(shaderc_target_env_vulkan, 0);

    if (pShaderFileReader) {
        assert(pOutIncludedFiles != nullptr && "Caught a requested included files set without includer.");
        options.SetIncluder(std::make_unique<Includer>(*pShaderFileReader, pOutIncludedFiles));
    }

    if (pMacros) {
        for (uint32_t i = 0; i < pMacros->GetCount(); ++i) {
            const auto macroDefinition = pMacros->GetMacroDefinition(i);
            options.AddMacroDefinition(std::string(macroDefinition.pszKey), std::string(macroDefinition.pszValue));

            // apemode::LogInfo("ShaderCompiler: Adding definition: {}={}", macroDefinition.pszKey,
            // macroDefinition.pszValue);
        }
    }

    std::string fullPath;
    std::string fileContent;

    // apemode::LogInfo("ShaderCompiler: Compiling {}", InFilePath);

    if (pShaderFileReader->ReadShaderTxtFile(InFilePath, fullPath, fileContent)) {
        auto compiledShader = InternalCompile(
            fullPath, fileContent, pMacros, eShaderKind, options, true, &Compiler, pShaderFeedbackWriter);
        if (compiledShader) {
            pOutIncludedFiles->InsertIncludedFile(fullPath);
            return compiledShader;
        }
    }

    return nullptr;
}

apemode::unique_ptr<IShaderCompiler> apemode::shp::NewShaderCompiler() {
    return apemode::unique_ptr<IShaderCompiler>(apemode_new ShaderCompiler{});
}
