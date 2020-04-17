#include "ShaderCompiler.h"

#include <apemode/platform/AppState.h>

#include <memory>
#include <shaderc/shaderc.hpp>
#include <spirv_glsl.hpp>
#include <spirv_msl.hpp>
#include <spirv_reflect.hpp>
using namespace apemode::shp;

namespace {

constexpr shaderc_shader_kind ToShaderKind(apemode::shp::IShaderCompiler::ShaderType type) {
    switch (type) { // clang-format off
        case apemode::shp::IShaderCompiler::ShaderType::Vertex: return shaderc_shader_kind::shaderc_vertex_shader; break;
        case apemode::shp::IShaderCompiler::ShaderType::Fragment: return shaderc_shader_kind::shaderc_fragment_shader; break;
        case apemode::shp::IShaderCompiler::ShaderType::Compute: return shaderc_shader_kind::shaderc_compute_shader; break;
        case apemode::shp::IShaderCompiler::ShaderType::Geometry: return shaderc_shader_kind::shaderc_geometry_shader; break;
        case apemode::shp::IShaderCompiler::ShaderType::TessControl: return shaderc_shader_kind::shaderc_tess_control_shader; break;
        case apemode::shp::IShaderCompiler::ShaderType::TessEvaluation: return shaderc_shader_kind::shaderc_tess_evaluation_shader; break;
        case apemode::shp::IShaderCompiler::ShaderType::RayGeneration: return shaderc_shader_kind::shaderc_raygen_shader; break;
        case apemode::shp::IShaderCompiler::ShaderType::AnyHit: return shaderc_shader_kind::shaderc_anyhit_shader; break;
        case apemode::shp::IShaderCompiler::ShaderType::ClosestHit: return shaderc_shader_kind::shaderc_closesthit_shader; break;
        case apemode::shp::IShaderCompiler::ShaderType::Miss: return shaderc_shader_kind::shaderc_miss_shader; break;
        case apemode::shp::IShaderCompiler::ShaderType::Intersection: return shaderc_shader_kind::shaderc_intersection_shader; break;
        case apemode::shp::IShaderCompiler::ShaderType::Callable: return shaderc_shader_kind::shaderc_callable_shader; break;
        case apemode::shp::IShaderCompiler::ShaderType::Task: return shaderc_shader_kind::shaderc_task_shader; break;
        case apemode::shp::IShaderCompiler::ShaderType::Mesh: return shaderc_shader_kind::shaderc_mesh_shader; break;
        default: assert(false); return shaderc_shader_kind::shaderc_glsl_infer_from_source; break;
    } // clang-format on
}

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
    } // clang-format on
}
constexpr ReflectedPrimitiveType Reflect(spirv_cross::SPIRType::BaseType bt) {
    switch (bt) { // clang-format off
        case spirv_cross::SPIRType::Struct: return ReflectedPrimitiveType::Struct;
        case spirv_cross::SPIRType::Boolean: return ReflectedPrimitiveType::Bool;
        case spirv_cross::SPIRType::Char: return ReflectedPrimitiveType::Char;
        case spirv_cross::SPIRType::SByte: return ReflectedPrimitiveType::Char;
        case spirv_cross::SPIRType::UByte: return ReflectedPrimitiveType::UChar;
        case spirv_cross::SPIRType::Short: return ReflectedPrimitiveType::Short;
        case spirv_cross::SPIRType::UShort: return ReflectedPrimitiveType::UShort;
        case spirv_cross::SPIRType::Int: return ReflectedPrimitiveType::Int;
        case spirv_cross::SPIRType::UInt: return ReflectedPrimitiveType::UInt;
        case spirv_cross::SPIRType::Int64: return ReflectedPrimitiveType::Long;
        case spirv_cross::SPIRType::UInt64: return ReflectedPrimitiveType::ULong;
        case spirv_cross::SPIRType::Half: return ReflectedPrimitiveType::Half;
        case spirv_cross::SPIRType::Float: return ReflectedPrimitiveType::Float;
        case spirv_cross::SPIRType::Double: return ReflectedPrimitiveType::Double;
        case spirv_cross::SPIRType::Image: return ReflectedPrimitiveType::Image;
        case spirv_cross::SPIRType::SampledImage: return ReflectedPrimitiveType::SampledImage;
        case spirv_cross::SPIRType::Sampler: return ReflectedPrimitiveType::Sampler;
        case spirv_cross::SPIRType::AtomicCounter: return ReflectedPrimitiveType::AtomicCounter;
        case spirv_cross::SPIRType::AccelerationStructureNV: return ReflectedPrimitiveType::AccelerationStructure;
        case spirv_cross::SPIRType::ControlPointArray: return ReflectedPrimitiveType::ControlPointArray;
        default: assert(false); return ReflectedPrimitiveType::Error;
    } // clang-format on
}
constexpr size_t PrimitiveTypeSize(ReflectedPrimitiveType primitiveType) {
    switch (primitiveType) { // clang-format off
        case ReflectedPrimitiveType::Bool: return 1;
        case ReflectedPrimitiveType::Char: return 1;
        case ReflectedPrimitiveType::UChar: return 1;
        case ReflectedPrimitiveType::Short: return 2;
        case ReflectedPrimitiveType::UShort: return 2;
        case ReflectedPrimitiveType::Int: return 4;
        case ReflectedPrimitiveType::UInt: return 4;
        case ReflectedPrimitiveType::Long: return 8;
        case ReflectedPrimitiveType::ULong: return 8;
        case ReflectedPrimitiveType::Half: return 2;
        case ReflectedPrimitiveType::Float: return 4;
        case ReflectedPrimitiveType::Double: return 8;
        default: return 0;
    } // clang-format on
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

        assert(total_size >= reflectedType.EffectiveByteSize && "Caught size miscalculation.");
        if (uint32_t padding = total_size - reflectedType.EffectiveByteSize) {
            outputString += ", effective_size=" + std::to_string(reflectedType.EffectiveByteSize);
            outputString += ", total_size=" + std::to_string(total_size);
            outputString += ", padding=" + std::to_string(padding);
        } else {
            outputString += ", size=" + std::to_string(reflectedType.EffectiveByteSize);
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
    } else {
        outputString += ", is_active=" + std::string(reflectedResource.bIsActive ? "yes" : "no");
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

    std::string Strings[(uint32_t)CompiledShaderTarget::Count] = {};
    std::string Errors[(uint32_t)CompiledShaderTarget::Count] = {};

    spirv_cross::CompilerGLSL CompilerGLSL;
    spirv_cross::Compiler& Reflection;
    apemode::shp::ReflectedShader Reflected = {};

    template <typename C, typename E>
    static void CrossCompileOrCatchError(C&& compile, E&& err) {
        try {
            compile();
        }
        // clang-format off
        catch (std::exception e) { err(e.what()); }
        catch (std::bad_cast e) { err(e.what()); }
        catch (std::bad_alloc e) { err(e.what()); }
        catch (std::runtime_error e) { err(e.what()); }
        catch (spirv_cross::CompilerError e) { err(e.what()); }
        catch (...) { err("<Unexpected error>"); }
        // clang-format on
    }

    CompiledShader(std::vector<uint32_t>&& dwords, std::string&& preprocessedSrc, std::string&& assemblySrc)
        : Dwords(std::move(dwords)), CompilerGLSL(Dwords.data(), Dwords.size()), Reflection(CompilerGLSL) {
        Strings[(uint32_t)CompiledShaderTarget::Preprocessed] = std::move(preprocessedSrc);
        Strings[(uint32_t)CompiledShaderTarget::Assembly] = std::move(assemblySrc);

        CrossCompileOrCatchError(
            [&] {
                spirv_cross::CompilerGLSL::Options vulkanOptions = {};
                vulkanOptions.vulkan_semantics = true;
                CompilerGLSL.set_common_options(vulkanOptions);
                Strings[(uint32_t)CompiledShaderTarget::Vulkan] = CompilerGLSL.compile();
                PopulateReflection();
            },
            [&](std::string err) {
                apemode::LogError("Failed to compile for Vulkan: {}", err);
                Errors[(uint32_t)CompiledShaderTarget::Vulkan] = std::move(err);
            });

        CrossCompileOrCatchError(
            [&] {
                spirv_cross::CompilerMSL mslCompiler(Dwords.data(), Dwords.size());
                spirv_cross::CompilerMSL::Options options = {};
                options.platform = spirv_cross::CompilerMSL::Options::iOS;
                mslCompiler.set_msl_options(options);
                Strings[(uint32_t)CompiledShaderTarget::iOS] = mslCompiler.compile();
            },
            [&](std::string err) {
                apemode::LogError("Failed to compile for iOS: {}", err);
                Errors[(uint32_t)CompiledShaderTarget::iOS] = std::move(err);
            });

        CrossCompileOrCatchError(
            [&] {
                spirv_cross::CompilerMSL mslCompiler(Dwords.data(), Dwords.size());
                spirv_cross::CompilerMSL::Options options = {};
                options.platform = spirv_cross::CompilerMSL::Options::macOS;
                mslCompiler.set_msl_options(options);
                Strings[(uint32_t)CompiledShaderTarget::macOS] = mslCompiler.compile();
            },
            [&](std::string err) {
                apemode::LogError("Failed to compile for macOS: {}", err);
                Errors[(uint32_t)CompiledShaderTarget::macOS] = std::move(err);
            });

        CrossCompileOrCatchError(
            [&] {
                spirv_cross::CompilerGLSL glslCompiler(Dwords.data(), Dwords.size());
                spirv_cross::CompilerGLSL::Options options = {};
                options.es = true;
                options.version = 100;
                glslCompiler.set_common_options(options);
                Strings[(uint32_t)CompiledShaderTarget::ES2] = glslCompiler.compile();
            },
            [&](std::string err) {
                apemode::LogError("Failed to compile for ES 2.0: {}", err);
                Errors[(uint32_t)CompiledShaderTarget::ES2] = std::move(err);
            });

        CrossCompileOrCatchError(
            [&] {
                spirv_cross::CompilerGLSL glslCompiler(Dwords.data(), Dwords.size());
                spirv_cross::CompilerGLSL::Options options = {};
                options.es = true;
                options.version = 300;
                glslCompiler.set_common_options(options);
                Strings[(uint32_t)CompiledShaderTarget::ES3] = glslCompiler.compile();
            },
            [&](std::string err) {
                apemode::LogError("Failed to compile for ES 3.0: {}", err);
                Errors[(uint32_t)CompiledShaderTarget::ES3] = std::move(err);
            });
    }

    ~CompiledShader() = default;

    bool HasSourceFor(CompiledShaderTarget target) const override { return Strings[(uint32_t)target].empty() == false; }
    std::string_view GetSourceFor(CompiledShaderTarget target) const override { return Strings[(uint32_t)target]; }
    std::string_view GetErrorFor(CompiledShaderTarget target) const override { return Errors[(uint32_t)target]; }
    
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
        if (reflectedType.ElementPrimitiveType == ReflectedPrimitiveType::Struct) {
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

        if (reflectedType.ElementPrimitiveType == ReflectedPrimitiveType::Struct) {
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
        if (reflectedResource.Type.ElementPrimitiveType != ReflectedPrimitiveType::Struct) { return; }
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

    void ReflectResource(const spirv_cross::Resource& resource,
                         ReflectedResource& reflectedResource,
                         const std::unordered_set<spirv_cross::VariableID>& activeVariableIds,
                         bool bMightHaveActiveRanges) {
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
        
        auto entryPoints = Reflection.get_entry_points_and_stages();
        
        
        
        const auto& constants = Reflection.get_specialization_constants();
        ReflectConstantVector(constants, Reflected.Constants);

        // clang-format off
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
        // clang-format on

        PrintReflection(Reflected);
    }

    ReflectedShader&& GetReflection() override { return std::move(Reflected); };
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
                                       const shaderc_include_type includeType,
                                       const char* pszRequestingSource,
                                       const size_t includeDepth) {
        auto userData = std::make_unique<UserData>();
        if (userData && pIncludedFiles &&
            FileReader.ReadShaderTxtFile(
                pszRequestedSource, userData->Path, userData->Content, includeType == shaderc_include_type_relative)) {
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

    std::unique_ptr<ICompiledShader> Compile(const std::string& shaderName,
                                             const std::string& shaderCode,
                                             const IMacroDefinitionCollection* pMacros,
                                             ShaderType shaderType,
                                             ShaderOptimizationType optimizationType) const override;

    /* @note Compiling from source files */

    IShaderFileReader* GetShaderFileReader() override;
    IShaderFeedbackWriter* GetShaderFeedbackWriter() override;
    void SetShaderFileReader(IShaderFileReader* pShaderFileReader) override;
    void SetShaderFeedbackWriter(IShaderFeedbackWriter* pShaderFeedbackWriter) override;

    std::unique_ptr<ICompiledShader> Compile(const std::string& FilePath,
                                             const IMacroDefinitionCollection* pMacros,
                                             ShaderType shaderType,
                                             ShaderOptimizationType optimizationType,
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
    const IShaderCompiler::ShaderType shaderType,
    shaderc::CompileOptions& options,
    const bool bAssembly,
    const shaderc::Compiler* pCompiler,
    ShaderCompiler::IShaderFeedbackWriter* pShaderFeedbackWriter) {
    using namespace apemode::shp;
    if (nullptr == pCompiler) { return nullptr; }

    shaderc::PreprocessedSourceCompilationResult preprocessedSourceCompilationResult =
        pCompiler->PreprocessGlsl(shaderContent, ToShaderKind(shaderType), shaderName.c_str(), options);

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
        preprocessedSourceCompilationResult.begin(), ToShaderKind(shaderType), shaderName.c_str(), options);

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
        preprocessedSourceCompilationResult.begin(), ToShaderKind(shaderType), shaderName.c_str(), options);

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

    // clang-format off
    std::vector<uint32_t> dwords(spvCompilationResult.cbegin(), spvCompilationResult.cend());
    std::string preprocessedSrc(preprocessedSourceCompilationResult.cbegin(), preprocessedSourceCompilationResult.cend());
    std::string assemblySrc(assemblyCompilationResult.cbegin(), assemblyCompilationResult.cend());
    return std::unique_ptr<ICompiledShader>(new CompiledShader(std::move(dwords), std::move(preprocessedSrc), std::move(assemblySrc)));
    // clang-format on
}

std::unique_ptr<apemode::shp::ICompiledShader> ShaderCompiler::Compile(
    const std::string& shaderName,
    const std::string& shaderContent,
    const IMacroDefinitionCollection* pMacros,
    const ShaderType shaderType,
    const ShaderOptimizationType optimizationType) const {
    shaderc::CompileOptions options;
    options.SetSourceLanguage(shaderc_source_language_glsl);
    options.SetOptimizationLevel(shaderc_optimization_level(optimizationType));
    options.SetTargetEnvironment(shaderc_target_env_vulkan, 0);
    options.SetGenerateDebugInfo();

    // clang-format off
    return InternalCompile(shaderName, shaderContent, pMacros, shaderType, options, true, &Compiler, pShaderFeedbackWriter);
    // clang-format on
}

std::unique_ptr<apemode::shp::ICompiledShader> ShaderCompiler::Compile(const std::string& filePath,
                                                                       const IMacroDefinitionCollection* pMacros,
                                                                       const ShaderType shaderType,
                                                                       const ShaderOptimizationType optimizationType,
                                                                       IIncludedFileSet* pOutIncludedFiles) const {
    // apemode::LogInfo("ShaderCompiler: Compiling {}", InFilePath);

    if (!pShaderFileReader) { return nullptr; }

    shaderc::CompileOptions options;
    options.SetSourceLanguage(shaderc_source_language_glsl);
    options.SetOptimizationLevel(shaderc_optimization_level(optimizationType));
    options.SetTargetEnvironment(shaderc_target_env_vulkan, 0);
    options.SetGenerateDebugInfo();

    if (pOutIncludedFiles) {
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

    switch (shaderType) { // clang-format off
        case ShaderType::Vertex:         options.AddMacroDefinition("VERTEX_SHADER", "1");          break;
        case ShaderType::Fragment:       options.AddMacroDefinition("FRAGMENT_SHADER", "1");        break;
        case ShaderType::Compute:        options.AddMacroDefinition("COMPUTE_SHADER", "1");         break;
        case ShaderType::Geometry:       options.AddMacroDefinition("GEOMETRY_SHADER", "1");        break;
        case ShaderType::TessControl:    options.AddMacroDefinition("TESSCONTROL_SHADER", "1");     break;
        case ShaderType::TessEvaluation: options.AddMacroDefinition("TESSEVALUATION_SHADER", "1");  break;
        case ShaderType::RayGeneration:  options.AddMacroDefinition("RAYGENERATION_SHADER", "1");   break;
        case ShaderType::AnyHit:         options.AddMacroDefinition("ANYHIT_SHADER", "1");          break;
        case ShaderType::ClosestHit:     options.AddMacroDefinition("CLOSESTHIT_SHADER", "1");      break;
        case ShaderType::Miss:           options.AddMacroDefinition("MISS_SHADER", "1");            break;
        case ShaderType::Intersection:   options.AddMacroDefinition("INTERSECTION_SHADER", "1");    break;
        case ShaderType::Callable:       options.AddMacroDefinition("CALLABLE_SHADER", "1");        break;
        case ShaderType::Task:           options.AddMacroDefinition("TASK_SHADER", "1");            break;
        case ShaderType::Mesh:           options.AddMacroDefinition("MESH_SHADER", "1");            break;
        default:                         options.AddMacroDefinition("ANY_SHADER", "1");             break;
    } // clang-format on

    std::string fullPath = "";
    std::string contents = "";
    if (pShaderFileReader->ReadShaderTxtFile(filePath, fullPath, contents, true)) { // clang-format off
        if (auto compiledShader = InternalCompile(fullPath, contents, pMacros, shaderType, options, true, &Compiler, pShaderFeedbackWriter)) {
            pOutIncludedFiles->InsertIncludedFile(fullPath);
            return compiledShader;
        }
    } // clang-format on

    return nullptr;
}

std::unique_ptr<IShaderCompiler> apemode::shp::NewShaderCompiler() {
    return std::unique_ptr<IShaderCompiler>(new ShaderCompiler{});
}
