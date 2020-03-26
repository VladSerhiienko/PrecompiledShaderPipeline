#include "ShaderCompiler.Vulkan.h"

#include <apemode/platform/AppState.h>
#include <apemode/platform/IAssetManager.h>

#include <shaderc/shaderc.hpp>
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
        case spirv_cross::SPIRType::AtomicCounter: return eReflectedPrimitiveType_AtomicCounter;
        case spirv_cross::SPIRType::AccelerationStructureNV: return eReflectedPrimitiveType_AccelerationStructure;
        case spirv_cross::SPIRType::ControlPointArray: return eReflectedPrimitiveType_ControlPointArray;
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

    outputString += reflectedType.Name;

    if (offset != -1 && total_size != -1) {
        outputString += " (offset=" + std::to_string(offset);
        outputString += ", size=" + std::to_string(total_size);

        assert(total_size >= reflectedType.EffectiveByteSize && "Caught size miscalculation.");
        if (uint32_t padding = total_size - reflectedType.EffectiveByteSize) {
            outputString += ", padding=" + std::to_string(padding);
        }
        outputString += ")";
    } else if (reflectedType.EffectiveByteSize) {
        outputString += " (size=" + std::to_string(reflectedType.EffectiveByteSize);
        outputString += ")";
    }

    apemode::LogInfo("{}", outputString);

    if (!reflectedType.Members.empty()) {
        uint32_t memberCount = reflectedType.Members.size();
        for (uint32_t i = 0; i < memberCount; ++i) {
            PrintReflection(reflectedType.Members[i]->Type,
                            prefix + (!isLastItem ? "│   " : "    "),
                            (i + 1) == memberCount,
                            reflectedType.Members[i]->Name,
                            reflectedType.Members[i]->ByteOffset,
                            reflectedType.Members[i]->OccupiedByteSize);
        }
    }
}
void PrintReflection(const ReflectedResource& reflectedResource, const std::string& prefix, bool isLastItem) {
    std::string outputString = prefix;
    
    // clang-format off
    outputString += isLastItem ? "└ - " : "├ - ";
    outputString += reflectedResource.Name.empty() ? "<unnamed>" : reflectedResource.Name;
    outputString += " (set=" + ((reflectedResource.DecorationDescriptorSet == -1) ? "?" : std::to_string(reflectedResource.DecorationDescriptorSet));
    outputString += ", binding=" + ((reflectedResource.DecorationBinding == -1) ? "?" : std::to_string(reflectedResource.DecorationBinding));
    outputString += ", location=" + ((reflectedResource.DecorationLocation == -1) ? "?" : std::to_string(reflectedResource.DecorationLocation));
    outputString += ", active=" + std::string(reflectedResource.bIsActive ? "yes" : "no");
    
    if (!reflectedResource.ActiveRanges.empty()) {
        outputString += ", active_ranges=[";
        for (auto& r : reflectedResource.ActiveRanges) {
            outputString += "(";
            outputString += std::to_string(r.offset);
            outputString += ":";
            outputString += std::to_string(r.size);
            outputString += "),";
        }
        
        outputString.back() = ']';
    }
    
    outputString += ")";
    // clang-format on

    apemode::LogInfo("{}", outputString);
    PrintReflection(reflectedResource.Type, prefix + (!isLastItem ? "│   " : "    "), true);
}
void PrintReflection(const ReflectedConstant& reflectedConstant, const std::string& prefix, bool last) {
    std::string outputString = prefix;
    outputString += last ? "└ - " : "├ - ";
    outputString += reflectedConstant.Name;

    apemode::LogInfo("{}", outputString);
    PrintReflection(reflectedConstant.Type, prefix + (!last ? "│   " : "    "), true);
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
    PrintReflection(reflectedShader.Constants);
    PrintReflection(reflectedShader.StageInputs, "stage inputs");
    PrintReflection(reflectedShader.StageOutputs, "stage outputs");
    PrintReflection(reflectedShader.UniformBuffers, "uniform buffers");
    PrintReflection(reflectedShader.PushConstantBuffers, "push constants");
    PrintReflection(reflectedShader.SampledImages, "sampled images");
    PrintReflection(reflectedShader.SubpassInputs, "subpass inputs");
    PrintReflection(reflectedShader.StorageImages, "storage images");
    PrintReflection(reflectedShader.StorageBuffers, "storage buffers");
    PrintReflection(reflectedShader.SeparateImages, "images");
    PrintReflection(reflectedShader.SeparateSamplers, "samplers");
}
} // namespace

ReflectedType::ReflectedType() = default;
ReflectedType::ReflectedType(ReflectedType&&) noexcept = default;
ReflectedType& ReflectedType::operator=(ReflectedType&&) = default;
ReflectedType::~ReflectedType() = default;

class CompiledShader : public ICompiledShader {
public:
    std::vector<uint32_t> Dwords = {};
    std::string PreprocessedSrc = {};
    std::string AssemblySrc = {};
    std::string CompiledGLSL = {};
    std::string CompiledMSL = {};
    apemode::shp::ReflectedShader Reflected = {};

    spirv_cross::CompilerGLSL CompilerGLSL;
    spirv_cross::CompilerMSL CompilerMSL;
    spirv_cross::Compiler& Reflection;

    CompiledShader(std::vector<uint32_t>&& dwords,
                   std::string&& preprocessedSrc,
                   std::string&& assemblySrc)
        : Dwords(std::move(dwords))
        , PreprocessedSrc(std::move(preprocessedSrc))
        , AssemblySrc(std::move(assemblySrc))
        , CompilerGLSL(Dwords.data(), Dwords.size())
        , CompilerMSL(Dwords.data(), Dwords.size())
        , Reflection(CompilerGLSL) { // Dwords.data(), Dwords.size()) {

        spirv_cross::CompilerGLSL::Options glslOptions = {};
        glslOptions.vulkan_semantics = true;
        // glslOptions.es = true;
        // glslOptions.version = 100;
        // glslOptions.vulkan_semantics = false;
        CompilerGLSL.set_common_options(glslOptions);

        CompiledGLSL = CompilerGLSL.compile();
        CompiledMSL = CompilerMSL.compile();
        PopulateReflection();
    }

    ~CompiledShader() {}

    void ReflectMemberType(const spirv_cross::SPIRType& type,
                           ReflectedType& reflectedType,
                           uint32_t member_type_index) {
        reflectedType.Members[member_type_index].reset(new ReflectedStructMember());
        auto& reflectedMemberType = *reflectedType.Members[member_type_index];

        const spirv_cross::SPIRType& memberType = Reflection.get_type(type.member_types[member_type_index]);
        reflectedMemberType.Name = Reflection.get_member_name(type.self, member_type_index);
        reflectedMemberType.ByteOffset = Reflection.type_struct_member_offset(type, member_type_index);
        reflectedMemberType.EffectiveByteSize = Reflection.get_declared_struct_member_size(type, member_type_index);
        reflectedMemberType.OccupiedByteSize = reflectedMemberType.EffectiveByteSize;

        reflectedMemberType.Type = ReflectType(memberType, reflectedMemberType.EffectiveByteSize);

        if (!memberType.array.empty()) {
            const uint32_t arrayStride = Reflection.type_struct_member_array_stride(type, member_type_index);
            assert(arrayStride == reflectedMemberType.Type.ArrayByteStride && "Caught paddings.");
            reflectedMemberType.Type.ArrayByteStride = arrayStride;
        }

        if (memberType.columns > 1) {
            uint32_t matrixStride = Reflection.type_struct_member_matrix_stride(type, member_type_index);
            assert(matrixStride == reflectedMemberType.Type.ElementMatrixByteStride && "Caught paddings.");
            reflectedMemberType.Type.ElementMatrixByteStride = matrixStride;
        }
    }

    ReflectedType ReflectType(const spirv_cross::SPIRType& type, uint32_t memberEffectiveSize = 0) {
        ReflectedType reflectedType = {};

        reflectedType.Name = Reflection.get_name(type.self);

        reflectedType.ArrayLength = 1;
        reflectedType.bIsArrayLengthStatic = false;
        reflectedType.ElementVectorLength = std::max<uint32_t>(1, type.vecsize);
        reflectedType.ElementColumnCount = std::max<uint32_t>(1, type.columns);
        reflectedType.ElementPrimitiveType = Reflect(type.basetype);

        // clang-format off
        if (reflectedType.ElementPrimitiveType == eReflectedPrimitiveType_Struct) {
            reflectedType.ElementByteSize = Reflection.get_declared_struct_size(type);
        } else {
            reflectedType.ElementByteSize = PrimitiveTypeSize(reflectedType.ElementPrimitiveType) *
                                              reflectedType.ElementVectorLength *
                                              reflectedType.ElementColumnCount;
        }

        if (!type.array.empty()) {
            reflectedType.bIsArrayLengthStatic = type.array_size_literal[0];
            if (reflectedType.bIsArrayLengthStatic) {
                reflectedType.ArrayLength = type.array[0];
                assert(reflectedType.ArrayLength == (memberEffectiveSize / reflectedType.ElementByteSize) && "Caught array size mismatch.");
            } else {
                reflectedType.ArrayLength = memberEffectiveSize / reflectedType.ElementByteSize;
            }
        }
        // clang-format on

        reflectedType.ElementMatrixByteStride = reflectedType.ElementByteSize / reflectedType.ElementColumnCount;
        reflectedType.ArrayByteStride = reflectedType.ElementByteSize;
        reflectedType.EffectiveByteSize = reflectedType.ArrayLength * reflectedType.ArrayByteStride;

        if (reflectedType.ElementPrimitiveType == eReflectedPrimitiveType_Struct) {
            const size_t member_count = type.member_types.size();
            reflectedType.Members.resize(member_count);
            for (size_t i = 0; i < member_count; i++) { ReflectMemberType(type, reflectedType, i); }

            for (size_t i = 0; i < (member_count - 1); i++) {
                const uint32_t nextOffset = reflectedType.Members[i + 1]->ByteOffset;
                const uint32_t thisOffset = reflectedType.Members[i]->ByteOffset;
                reflectedType.Members[i]->OccupiedByteSize = nextOffset - thisOffset;
            }
        }

        if (reflectedType.Name.empty()) {
            reflectedType.Name = ToString(type.basetype);

            if (reflectedType.ElementVectorLength > 1 && reflectedType.ElementColumnCount > 1) {
                reflectedType.Name += std::to_string(reflectedType.ElementVectorLength);
                reflectedType.Name += "x";
                reflectedType.Name += std::to_string(reflectedType.ElementColumnCount);
            } else if (reflectedType.ElementVectorLength > 1 || reflectedType.ElementColumnCount > 1) {
                const auto length = std::max(reflectedType.ElementVectorLength, reflectedType.ElementColumnCount);
                reflectedType.Name += std::to_string(length);
            }

            if (reflectedType.ArrayLength > 1) {
                reflectedType.Name += "[";
                reflectedType.Name += std::to_string(reflectedType.ArrayLength);
                reflectedType.Name += reflectedType.bIsArrayLengthStatic ? "|!]" : "|?]";
            }
        }

        return reflectedType;
    }

    uint32_t ReflectDecoration(spirv_cross::ID resourceId, spv::Decoration decoration, uint32_t missingValue = -1) {
        if (!Reflection.has_decoration(resourceId, decoration)) { return missingValue; }
        return Reflection.get_decoration(resourceId, decoration);
    }

    void ReflectResourceActiveRanges(const spirv_cross::Resource& resource, ReflectedResource& reflectedResource) {
        if (reflectedResource.Type.ElementPrimitiveType != eReflectedPrimitiveType_Struct) { return; }
        auto activeRanges = Reflection.get_active_buffer_ranges(resource.id);
        if (!activeRanges.empty()) {
        
            // clang-format off
            reflectedResource.ActiveRanges.reserve(activeRanges.size());
            for (auto& r : activeRanges) { reflectedResource.ActiveRanges.push_back({(uint32_t)r.offset, (uint32_t)r.range}); }
            std::sort(reflectedResource.ActiveRanges.begin(), reflectedResource.ActiveRanges.end(), [](ReflectedMemoryRange r0, ReflectedMemoryRange r1){ return r0.offset < r1.offset; });
            // clang-format on

            //
            // Collapse into minimal amount of memory ranges.
            //
            
            if (reflectedResource.ActiveRanges.size() > 1) {
                auto prevIt = reflectedResource.ActiveRanges.begin();
                auto currIt = std::next(prevIt);
                while (currIt != reflectedResource.ActiveRanges.end()) {
                    if (currIt->offset == (prevIt->offset + prevIt->size)) {
                        prevIt->size += currIt->size;
                        currIt = reflectedResource.ActiveRanges.erase(currIt);
                    } else {
                        prevIt = currIt;
                        currIt = std::next(prevIt);
                    }
                }
            }
        }
    }

    void ReflectResource(const spirv_cross::Resource& resource, ReflectedResource& reflectedResource, const std::unordered_set<spirv_cross::VariableID>& activeVariableIds, bool bMightHaveActiveRanges) {
        const spirv_cross::SPIRType& type = Reflection.get_type(resource.base_type_id);
        reflectedResource.Type = ReflectType(type);
        reflectedResource.Name = Reflection.get_name(resource.id);
        reflectedResource.DecorationDescriptorSet = ReflectDecoration(resource.id, spv::DecorationDescriptorSet);
        reflectedResource.DecorationBinding = ReflectDecoration(resource.id, spv::DecorationBinding);
        reflectedResource.DecorationLocation = ReflectDecoration(resource.id, spv::DecorationLocation);
        
        const auto it = activeVariableIds.find(resource.id);
        if (it != activeVariableIds.end()) {
            reflectedResource.bIsActive = true;
            ReflectResourceActiveRanges(resource, reflectedResource);
        }
    }

    void ReflectResourceVector(const spirv_cross::SmallVector<spirv_cross::Resource>& resources,
                               std::vector<ReflectedResource>& reflectedResources,
                               const std::unordered_set<spirv_cross::VariableID>& activeVariableIds,
                               bool bMightHaveActiveRanges = false) {
        reflectedResources.resize(resources.size());
        for (size_t i = 0; i < resources.size(); ++i) {
            const spirv_cross::Resource& resource = resources[i];
            auto& reflectedResource = reflectedResources[i];
            ReflectResource(resource, reflectedResource, activeVariableIds, bMightHaveActiveRanges);
        }
    }

    void ReflectConstant(const spirv_cross::SpecializationConstant& constant, ReflectedConstant& reflectedConstant) {
        const spirv_cross::SPIRConstant& spirConstant = Reflection.get_constant(constant.id);
        const spirv_cross::SPIRType& type = Reflection.get_type(spirConstant.constant_type);
        reflectedConstant.Name = Reflection.get_name(constant.id);
        reflectedConstant.MacroName = spirConstant.specialization_constant_macro_name;
        reflectedConstant.Type = ReflectType(type);
        reflectedConstant.ConstantId = constant.constant_id;
        reflectedConstant.bIsSpecialization = spirConstant.specialization;
        reflectedConstant.bIsUsedAsArrayLength = spirConstant.is_used_as_array_length;
        reflectedConstant.bIsUsedAsLUT = spirConstant.is_used_as_lut;
        
        if (reflectedConstant.MacroName.empty()) {
            constexpr std::string_view kSpirvCrossConstantIdPrefix = "SPIRV_CROSS_CONSTANT_ID_";
            reflectedConstant.MacroName = kSpirvCrossConstantIdPrefix;
            reflectedConstant.MacroName += std::to_string(reflectedConstant.ConstantId);
        }

        reflectedConstant.DefaultValue.u64 = 0;
        const uint64_t defaultScalar = spirConstant.scalar_u64();
        const size_t effectiveBytes = reflectedConstant.Type.EffectiveByteSize;
        memcpy(reflectedConstant.DefaultValue.u8, &defaultScalar, effectiveBytes);
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
        ReflectConstantVector(constants, Reflected.Constants);
        
        const auto activeVariableIds = Reflection.get_active_interface_variables();
        const spirv_cross::ShaderResources shaderResources = Reflection.get_shader_resources();
        ReflectResourceVector(shaderResources.uniform_buffers, Reflected.UniformBuffers, activeVariableIds, true);
        ReflectResourceVector(shaderResources.push_constant_buffers, Reflected.PushConstantBuffers, activeVariableIds, true);
        ReflectResourceVector(shaderResources.stage_inputs, Reflected.StageInputs, activeVariableIds);
        ReflectResourceVector(shaderResources.stage_outputs, Reflected.StageOutputs, activeVariableIds);
        ReflectResourceVector(shaderResources.sampled_images, Reflected.SampledImages, activeVariableIds);
        ReflectResourceVector(shaderResources.separate_images, Reflected.SeparateImages, activeVariableIds);
        ReflectResourceVector(shaderResources.separate_samplers, Reflected.SeparateSamplers, activeVariableIds);
        ReflectResourceVector(shaderResources.storage_images, Reflected.StorageImages, activeVariableIds);
        ReflectResourceVector(shaderResources.storage_buffers, Reflected.StorageBuffers, activeVariableIds, true);

        PrintReflection(Reflected);
    }

    // clang-format off
    std::string_view GetPreprocessedSrc() const override { std::string_view sv{PreprocessedSrc.c_str(), PreprocessedSrc.size()}; return sv; }
    std::string_view GetAssemblySrc() const override { std::string_view sv{AssemblySrc.c_str(), AssemblySrc.size()}; return sv; }
    std::string_view GetCompiledGLSL() const override { std::string_view sv{CompiledGLSL.c_str(), CompiledGLSL.size()}; return sv; }
    std::string_view GetCompiledMSL() const override { std::string_view sv{CompiledMSL.c_str(), CompiledMSL.size()}; return sv; }
    ReflectedShader&& GetReflection() override { return std::move(Reflected); };
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
        auto userData = apemode::make_unique<UserData>();
        if (userData && pIncludedFiles &&
            FileReader.ReadShaderTxtFile(pszRequestedSource, userData->Path, userData->Content)) {
            pIncludedFiles->InsertIncludedFile(userData->Path);

            auto includeResult = std::make_unique<shaderc_include_result>();
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
        delete ((UserData*&)data->user_data);
        delete data;
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

    std::unique_ptr<ICompiledShader> Compile(const std::string& ShaderName,
                                             const std::string& ShaderCode,
                                             const IMacroDefinitionCollection* pMacros,
                                             ShaderType eShaderKind,
                                             EShaderOptimizationType eShaderOptimization) const override;

    /* @note Compiling from source files */

    IShaderFileReader* GetShaderFileReader() override;
    IShaderFeedbackWriter* GetShaderFeedbackWriter() override;
    void SetShaderFileReader(IShaderFileReader* pShaderFileReader) override;
    void SetShaderFeedbackWriter(IShaderFeedbackWriter* pShaderFeedbackWriter) override;

    std::unique_ptr<ICompiledShader> Compile(const std::string& FilePath,
                                             const IMacroDefinitionCollection* pMacros,
                                             ShaderType eShaderKind,
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

static std::unique_ptr<apemode::shp::ICompiledShader> InternalCompile(
    const std::string& shaderName,
    const std::string& shaderContent,
    const IShaderCompiler::IMacroDefinitionCollection* pMacros,
    const IShaderCompiler::ShaderType eShaderKind,
    shaderc::CompileOptions& options,
    const bool bAssembly,
    const shaderc::Compiler* pCompiler,
    ShaderCompiler::IShaderFeedbackWriter* pShaderFeedbackWriter) {
    using namespace apemode::shp;
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
        pShaderFeedbackWriter->WriteFeedback(ShaderCompiler::IShaderFeedbackWriter::eFeedback_SpvSucceeded,
                                             shaderName,
                                             pMacros,
                                             spvCompilationResult.cbegin(),
                                             spvCompilationResult.cend());
    }

    std::vector<uint32_t> dwords(spvCompilationResult.cbegin(), spvCompilationResult.cend());
    std::string preprocessedSrc(preprocessedSourceCompilationResult.cbegin(), preprocessedSourceCompilationResult.cend());
    std::string assemblySrc(assemblyCompilationResult.cbegin(), assemblyCompilationResult.cend());

    auto pCompiledShader = new CompiledShader(std::move(dwords), std::move(preprocessedSrc), std::move(assemblySrc));
    return std::unique_ptr<ICompiledShader>(pCompiledShader);
}

std::unique_ptr<apemode::shp::ICompiledShader> ShaderCompiler::Compile(
    const std::string& shaderName,
    const std::string& shaderContent,
    const IMacroDefinitionCollection* pMacros,
    const ShaderType eShaderKind,
    const EShaderOptimizationType eShaderOptimization) const {
    apemode_memory_allocation_scope;

    shaderc::CompileOptions options;
    options.SetSourceLanguage(shaderc_source_language_glsl);
    options.SetOptimizationLevel(shaderc_optimization_level(eShaderOptimization));
    options.SetTargetEnvironment(shaderc_target_env_vulkan, 0);

    return InternalCompile(shaderName, shaderContent, pMacros, eShaderKind, options, true, &Compiler, pShaderFeedbackWriter);
}

std::unique_ptr<apemode::shp::ICompiledShader> ShaderCompiler::Compile(
    const std::string& InFilePath,
    const IMacroDefinitionCollection* pMacros,
    const ShaderType eShaderKind,
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
        auto compiledShader = InternalCompile(fullPath, fileContent, pMacros, eShaderKind, options, true, &Compiler, pShaderFeedbackWriter);
        if (compiledShader) {
            pOutIncludedFiles->InsertIncludedFile(fullPath);
            return compiledShader;
        }
    }

    return nullptr;
}

std::unique_ptr<IShaderCompiler> apemode::shp::NewShaderCompiler() {
    return std::unique_ptr<IShaderCompiler>(new ShaderCompiler{});
}
