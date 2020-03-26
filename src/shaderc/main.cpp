
#include "ShaderCompiler.Vulkan.h"
#include "cso_generated.h"

#include <apemode/platform/AppState.h>
#include <apemode/platform/CityHash.h>
#include <apemode/platform/shared/AssetManager.h>

#include <flatbuffers/util.h>
#include <nlohmann/json.hpp>

#include <iterator>

using json = nlohmann::json;

namespace {
struct UniqueString {
    uint64_t Hash = 0;
    std::string Contents = "";
};

struct UniqueBuffer {
    uint64_t Hash = 0;
    std::vector<uint8_t> Contents = {};
};

struct CompiledShaderVariant {
    CompiledShaderVariant() = default;
    CompiledShaderVariant(CompiledShaderVariant&&) noexcept = default;
    CompiledShaderVariant& operator=(CompiledShaderVariant&&) = default;
    ~CompiledShaderVariant() = default;

    std::string Preprocessed = "";
    std::string Assembly = "";
    std::string Asset = "";
    std::string Definitions = "";
    std::string CompiledGLSL = "";
    std::string CompiledMSL = "";
    std::vector<uint8_t> Compiled = {};
    apemode::shp::ReflectedShader Reflected = {};
    cso::Shader Type = cso::Shader::Shader_MAX;
    std::set<std::string> IncludedFiles = {};
    std::map<std::string, std::string> DefinitionMap = {};
};

struct HashedCompiledShader {
    uint64_t Hash = 0;
    uint32_t PreprocessedIndex = 0;
    uint32_t AssemblyIndex = 0;
    uint32_t CompiledIndex = 0;
    uint32_t GLSLIndex = 0;
    uint32_t MSLIndex = 0;
    uint32_t ReflectedIndex = 0;
};

struct HashedCompiledShaderInfo {
    uint64_t Hash = 0;
    uint32_t AssetIndex = 0;
    uint32_t CompiledShaderIndex = 0;
    uint32_t DefinitionsIndex = 0;
    cso::Shader ShaderType = cso::Shader::Shader_MAX;
    std::vector<uint32_t> IncludedFileIndices;
    std::vector<uint32_t> DefinitionIndices;
};

struct HashedReflectedTypeMember {
    uint32_t NameIndex = 0;
    uint32_t TypeIndex = 0;
    uint32_t ByteOffset = 0;
    uint32_t EffectiveByteSize = 0;
    uint32_t OccupiedByteSize = 0;
};

struct HashedReflectedType {
    uint64_t Hash = 0;
    uint32_t NameIndex = 0;
    cso::ReflectedPrimitiveType ElementPrimitiveType = cso::ReflectedPrimitiveType::ReflectedPrimitiveType_MAX;
    uint32_t ElementByteSize = 0;
    uint32_t ElementVectorLength = 0;
    uint32_t ElementColumnCount = 0;
    uint32_t ElementMatrixByteStride = 0;
    uint32_t ArrayLength = 0;
    bool bIsArrayLengthStatic = false;
    uint32_t ArrayByteStride = 0;
    uint32_t EffectiveByteSize = 0;
    std::vector<HashedReflectedTypeMember> MemberTypes = {};
};

struct HashedReflectedResourceState {
    uint64_t Hash = 0;
    bool bIsActive = false;
    std::vector<std::pair<uint32_t, uint32_t>> ActiveRanges = {};
};

struct HashedReflectedResource {
    uint64_t Hash = 0;
    uint32_t NameIndex = 0;
    uint32_t TypeIndex = 0;
    uint32_t DescriptorSet = 0;
    uint32_t DescriptorBinding = 0;
    uint32_t Locaton = 0;
};

struct HashedReflectedConstant {
    uint64_t Hash = 0;
    uint32_t NameIndex = 0;
    uint32_t MacroIndex = 0;
    uint64_t DefaultScalarU64 = 0;
    uint32_t ConstantId = 0;
    uint32_t TypeIndex = 0;
    bool bIsSpecialization = false;
    bool bIsUsedAsArrayLength = false;
    bool bIsUsedAsLUT = false;
};

struct HashedReflectedShader {
    uint64_t Hash = 0;
    uint32_t NameIndex = 0;
    std::vector<uint32_t> ConstantIndices = {};
    std::vector<uint32_t> StageInputIndices = {};
    std::vector<uint32_t> StageOutputIndices = {};
    std::vector<uint32_t> UniformBufferIndices = {};
    std::vector<uint32_t> PushConstantBufferIndices = {};
    std::vector<uint32_t> SampledImageIndices = {};
    std::vector<uint32_t> SubpassImageIndices = {};
    std::vector<uint32_t> SeparateImageIndices = {};
    std::vector<uint32_t> SeparateSamplerIndices = {};
    std::vector<uint32_t> StorageImageIndices = {};
    std::vector<uint32_t> StorageBufferIndices = {};
    std::vector<uint32_t> UniformBufferStateIndices = {};
    std::vector<uint32_t> PushConstantBufferStateIndices = {};
    std::vector<uint32_t> StorageBufferStateIndices = {};
};

struct CompiledShaderCollection {
    std::vector<UniqueString> uniqueStrings = {};
    std::vector<UniqueBuffer> uniqueBuffers = {};
    std::vector<HashedCompiledShader> uniqueCompiledShaders = {};
    std::vector<HashedCompiledShaderInfo> uniqueCompiledShaderInfos = {};
    std::vector<HashedReflectedType> uniqueReflectedTypes = {};
    std::vector<HashedReflectedResourceState> uniqueReflectedResourceStates = {};
    std::vector<HashedReflectedResource> uniqueReflectedResources = {};
    std::vector<HashedReflectedConstant> uniqueReflectedConstants = {};
    std::vector<HashedReflectedShader> uniqueReflectedShaders = {};

    void Serialize(flatbuffers::FlatBufferBuilder& fbb, const std::vector<CompiledShaderVariant>& variants) {
        Pack(variants);

        // clang-format off
        flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<cso::UniqueBuffer>>> hashedBuffersOffset = 0;
        flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<cso::UniqueString>>> hashedStringsOffset = 0;
        flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<cso::ReflectedShader>>> reflectedShadersOffset = 0;
        flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<cso::ReflectedType>>> reflectedTypesOffset = 0;
        flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<cso::ReflectedResourceState>>> reflectedStatesOffset = 0;
        flatbuffers::Offset<flatbuffers::Vector<const cso::ReflectedResource*>> reflectedResourcesOffset = 0;
        flatbuffers::Offset<flatbuffers::Vector<const cso::ReflectedConstant*>> reflectedConstantsOffset = 0;
        flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<cso::CompiledShaderInfo>>> compiledShaderInfosOffset = 0;
        flatbuffers::Offset<flatbuffers::Vector<const cso::CompiledShader*>> compiledShadersOffset = 0;
        // clang-format on
        
        apemode::LogInfo("+ {} buffers", uniqueBuffers.size());
        apemode::LogInfo("+ {} string", uniqueStrings.size());
        apemode::LogInfo("+ {} compiled shaders", uniqueCompiledShaders.size());
        apemode::LogInfo("+ {} compiled shader infos", uniqueCompiledShaderInfos.size());
        apemode::LogInfo("+ {} reflected types", uniqueReflectedTypes.size());
        apemode::LogInfo("+ {} reflected resource states", uniqueReflectedResourceStates.size());
        apemode::LogInfo("+ {} reflected resources", uniqueReflectedResources.size());
        apemode::LogInfo("+ {} reflected constants", uniqueReflectedConstants.size());
        apemode::LogInfo("+ {} reflected shaders", uniqueReflectedShaders.size());

        std::vector<flatbuffers::Offset<cso::UniqueBuffer>> hashedBufferOffsets = {};
        for (auto& hashedBuffer : uniqueBuffers) {
            const int8_t* contentsPtr = (const int8_t*)hashedBuffer.Contents.data();
            const uint32_t contentsLen = hashedBuffer.Contents.size();
            flatbuffers::Offset<flatbuffers::Vector<int8_t>> contentsOffset = 0;
            contentsOffset = fbb.CreateVector(contentsPtr, contentsLen);
            hashedBufferOffsets.push_back(cso::CreateUniqueBuffer(fbb, contentsOffset));
        }

        hashedBuffersOffset = fbb.CreateVector(hashedBufferOffsets.data(), hashedBufferOffsets.size());

        std::vector<flatbuffers::Offset<cso::UniqueString>> hashedStringOffsets = {};
        for (auto& hashedString : uniqueStrings) {
            const char* contentsPtr = hashedString.Contents.c_str();
            const uint32_t contentsLen = hashedString.Contents.length();
            flatbuffers::Offset<flatbuffers::String> contentsOffset = 0;
            contentsOffset = fbb.CreateString(contentsPtr, contentsLen);
            hashedStringOffsets.push_back(cso::CreateUniqueString(fbb, contentsOffset));
        }

        hashedStringsOffset = fbb.CreateVector(hashedStringOffsets.data(), hashedStringOffsets.size());

        // clang-format off
        std::vector<flatbuffers::Offset<cso::ReflectedType>> reflectedTypeOffsets = {};
        for (auto& reflectedType : uniqueReflectedTypes) {
            std::vector<cso::ReflectedStructMember> reflectedStructMembers = {};
            for (const auto& m : reflectedType.MemberTypes) {
                reflectedStructMembers.push_back(cso::ReflectedStructMember(m.NameIndex, m.TypeIndex, m.EffectiveByteSize, m.OccupiedByteSize, m.ByteOffset));
            }

            flatbuffers::Offset<flatbuffers::Vector<const cso::ReflectedStructMember*>> reflectedStructMembersOffset = 0;
            reflectedStructMembersOffset = fbb.CreateVectorOfStructs(reflectedStructMembers.data(), reflectedStructMembers.size());

            uint32_t arrayLength = reflectedType.ArrayLength & cso::ArrayLength_ValueBits;
            arrayLength |= reflectedType.bIsArrayLengthStatic ? cso::ArrayLength_IsStaticBit : 0;

            reflectedTypeOffsets.push_back(cso::CreateReflectedType(fbb,
                                                                    reflectedType.NameIndex,
                                                                    reflectedType.ElementPrimitiveType,
                                                                    reflectedType.ElementByteSize,
                                                                    reflectedType.ElementVectorLength,
                                                                    reflectedType.ElementColumnCount,
                                                                    reflectedType.ElementMatrixByteStride,
                                                                    arrayLength,
                                                                    reflectedType.ArrayByteStride,
                                                                    reflectedType.EffectiveByteSize,
                                                                    reflectedStructMembersOffset));
        }

        std::vector<flatbuffers::Offset<cso::ReflectedResourceState>> reflectedStateOffsets = {};
        for (auto& s : this->uniqueReflectedResourceStates) {
            auto rangesOffset = fbb.CreateVectorOfStructs((const cso::MemoryRange*)s.ActiveRanges.data(), s.ActiveRanges.size());
            reflectedStateOffsets.push_back(cso::CreateReflectedResourceState(fbb, s.bIsActive, rangesOffset));
        }

        reflectedStatesOffset = fbb.CreateVector(reflectedStateOffsets);

        std::vector<cso::ReflectedResource> reflectedResources = {};
        for (auto& r : this->uniqueReflectedResources) {
            reflectedResources.push_back(cso::ReflectedResource(r.NameIndex, r.TypeIndex, r.DescriptorSet, r.DescriptorBinding, r.Locaton));
        }
        // clang-format on

        reflectedResourcesOffset = fbb.CreateVectorOfStructs(reflectedResources.data(), reflectedResources.size());

        std::vector<cso::ReflectedConstant> reflectedConstants = {};
        for (auto& c : uniqueReflectedConstants) {
            uint32_t bits = 0;

            if (c.bIsSpecialization) { bits |= cso::ReflectedConstantBit_IsSpecializationBit; }
            if (c.bIsUsedAsArrayLength) { bits |= cso::ReflectedConstantBit_IsUsedAsArrayLengthBit; }
            if (c.bIsUsedAsLUT) { bits |= cso::ReflectedConstantBit_IsUsedAsLUT; }
            
            reflectedConstants.push_back(cso::ReflectedConstant(c.NameIndex,
                                                                c.MacroIndex,
                                                                c.DefaultScalarU64,
                                                                c.ConstantId,
                                                                c.TypeIndex,
                                                                bits));
        }

        reflectedConstantsOffset = fbb.CreateVectorOfStructs(reflectedConstants.data(), reflectedConstants.size());

        std::vector<flatbuffers::Offset<cso::ReflectedShader>> reflectedShaderOffsets = {};
        for (auto& s : this->uniqueReflectedShaders) {
            auto constantIndicesOffset = fbb.CreateVector(s.ConstantIndices);
            auto stageInputIndicesOffset = fbb.CreateVector(s.StageInputIndices);
            auto stageOutputIndicesOffset = fbb.CreateVector(s.StageOutputIndices);
            auto uniformBufferIndicesOffset = fbb.CreateVector(s.UniformBufferIndices);
            auto pushConstantBufferIndicesOffset = fbb.CreateVector(s.PushConstantBufferIndices);
            auto sampleImageIndicesOffset = fbb.CreateVector(s.SampledImageIndices);
            auto subpassInputIndicesOffset = fbb.CreateVector(s.SubpassImageIndices);
            auto separateImageIndicesOffset = fbb.CreateVector(s.SeparateImageIndices);
            auto separateSamplerIndicesOffset = fbb.CreateVector(s.SeparateSamplerIndices);
            auto storageImageIndicesOffset = fbb.CreateVector(s.StorageImageIndices);
            auto storageBufferIndicesOffset = fbb.CreateVector(s.StorageBufferIndices);
            auto uniformBufferStateIndicesOffset = fbb.CreateVector(s.UniformBufferStateIndices);
            auto pushConstantBufferStateIndicesOffset = fbb.CreateVector(s.PushConstantBufferStateIndices);
            auto storageBufferStateIndicesOffset = fbb.CreateVector(s.StorageBufferStateIndices);

            reflectedShaderOffsets.push_back(cso::CreateReflectedShader(fbb,
                                                                        s.NameIndex,
                                                                        constantIndicesOffset,
                                                                        stageInputIndicesOffset,
                                                                        stageOutputIndicesOffset,
                                                                        uniformBufferIndicesOffset,
                                                                        pushConstantBufferIndicesOffset,
                                                                        sampleImageIndicesOffset,
                                                                        subpassInputIndicesOffset,
                                                                        separateImageIndicesOffset,
                                                                        separateSamplerIndicesOffset,
                                                                        storageImageIndicesOffset,
                                                                        storageBufferIndicesOffset,
                                                                        uniformBufferStateIndicesOffset,
                                                                        pushConstantBufferStateIndicesOffset,
                                                                        storageBufferStateIndicesOffset));
        }

        reflectedShadersOffset = fbb.CreateVector(reflectedShaderOffsets);

        std::vector<cso::CompiledShader> compiledShaderOffsets = {};
        for (auto& compiledShader : uniqueCompiledShaders) {
            compiledShaderOffsets.push_back(cso::CompiledShader(compiledShader.PreprocessedIndex,
                                                                compiledShader.AssemblyIndex,
                                                                compiledShader.CompiledIndex,
                                                                compiledShader.GLSLIndex,
                                                                compiledShader.MSLIndex,
                                                                compiledShader.ReflectedIndex,
                                                                cso::IR_SPIRV));
        }

        compiledShadersOffset = fbb.CreateVectorOfStructs(compiledShaderOffsets.data(), compiledShaderOffsets.size());

        std::vector<flatbuffers::Offset<cso::CompiledShaderInfo>> compiledShaderInfoOffsets = {};
        for (auto& compiledShaderInfo : uniqueCompiledShaderInfos) {
            flatbuffers::Offset<flatbuffers::Vector<uint32_t>> includedFilesOffset = fbb.CreateVector(
                compiledShaderInfo.IncludedFileIndices.data(), compiledShaderInfo.IncludedFileIndices.size());
            flatbuffers::Offset<flatbuffers::Vector<uint32_t>> definitionsOffset = fbb.CreateVector(
                compiledShaderInfo.DefinitionIndices.data(), compiledShaderInfo.DefinitionIndices.size());
            compiledShaderInfoOffsets.push_back(cso::CreateCompiledShaderInfo(fbb,
                                                                              cso::Shader(compiledShaderInfo.ShaderType),
                                                                              compiledShaderInfo.CompiledShaderIndex,
                                                                              compiledShaderInfo.AssetIndex,
                                                                              compiledShaderInfo.DefinitionsIndex,
                                                                              definitionsOffset,
                                                                              includedFilesOffset));
        }

        compiledShaderInfosOffset =
            fbb.CreateVector(compiledShaderInfoOffsets.data(), compiledShaderInfoOffsets.size());

        flatbuffers::Offset<cso::CompiledShaderCollection> collectionOffset =
            cso::CreateCompiledShaderCollection(fbb,
                                                cso::Version_Value,
                                                compiledShaderInfosOffset,
                                                compiledShadersOffset,
                                                reflectedShadersOffset,
                                                reflectedTypesOffset,
                                                reflectedResourcesOffset,
                                                reflectedConstantsOffset,
                                                reflectedStatesOffset,
                                                hashedStringsOffset,
                                                hashedBuffersOffset);

        FinishCompiledShaderCollectionBuffer(fbb, collectionOffset);
    }

    void Pack(const std::vector<CompiledShaderVariant>& variants) {
        for (auto& cso : variants) {
            HashedCompiledShader compiledShader = {};
            compiledShader.CompiledIndex = GetBufferIndex(cso.Compiled);
            compiledShader.GLSLIndex = GetStringIndex(cso.CompiledGLSL);
            compiledShader.MSLIndex = GetStringIndex(cso.CompiledMSL);
            compiledShader.AssemblyIndex = GetStringIndex(cso.Assembly);
            compiledShader.PreprocessedIndex = GetStringIndex(cso.Preprocessed);
            compiledShader.ReflectedIndex = GetReflectedShaderIndex(GetHashedReflectionShader(cso.Reflected));

            compiledShader.Hash = uniqueBuffers[compiledShader.CompiledIndex].Hash;

            uint32_t compiledShaderIndex = GetCompiledShaderIndex(compiledShader);

            apemode::CityHash64Wrapper city64 = {};
            HashedCompiledShaderInfo compiledShaderInfo = {};

            compiledShaderInfo.CompiledShaderIndex = compiledShaderIndex;
            compiledShaderInfo.AssetIndex = GetStringIndex(cso.Asset);
            compiledShaderInfo.ShaderType = cso.Type;
            compiledShaderInfo.DefinitionsIndex = GetStringIndex(cso.Definitions);

            city64.CombineWith(compiledShaderInfo.CompiledShaderIndex);
            city64.CombineWith(compiledShaderInfo.AssetIndex);
            city64.CombineWith(compiledShaderInfo.ShaderType);
            city64.CombineWith(compiledShaderInfo.DefinitionsIndex);

            for (auto& includedFile : cso.IncludedFiles) {
                compiledShaderInfo.IncludedFileIndices.push_back(GetStringIndex(includedFile));
                city64.CombineWith(compiledShaderInfo.IncludedFileIndices.back());
            }

            for (auto& definitionPair : cso.DefinitionMap) {
                compiledShaderInfo.DefinitionIndices.push_back(GetStringIndex(definitionPair.first));
                city64.CombineWith(compiledShaderInfo.DefinitionIndices.back());

                compiledShaderInfo.DefinitionIndices.push_back(GetStringIndex(definitionPair.second));
                city64.CombineWith(compiledShaderInfo.DefinitionIndices.back());
            }

            compiledShaderInfo.Hash = city64.Value;
            AddCompiledShaderInfoIndex(compiledShaderInfo);
        }
    }

    // clang-format off
    void AddCompiledShaderInfoIndex(const HashedCompiledShaderInfo& compiledShaderInfo) {
        auto it = std::find_if(uniqueCompiledShaderInfos.begin(), uniqueCompiledShaderInfos.end(), [compiledShaderInfo](const HashedCompiledShaderInfo& existingCompiledShaderInfo) { return existingCompiledShaderInfo.Hash == compiledShaderInfo.Hash; });
        if (it == uniqueCompiledShaderInfos.end()) { uniqueCompiledShaderInfos.push_back(compiledShaderInfo); }
    }
    HashedReflectedResourceState GetHashedReflectedResourceState(const apemode::shp::ReflectedResource& reflectedResource) {
        HashedReflectedResourceState reflectedState = {};
        
        reflectedState.bIsActive = reflectedResource.bIsActive;
        reflectedState.ActiveRanges.reserve(reflectedResource.ActiveRanges.size());
        
        // clang-format off
        std::transform(reflectedResource.ActiveRanges.begin(),
                       reflectedResource.ActiveRanges.end(), std::back_inserter(reflectedState.ActiveRanges),
                       [](apemode::shp::ReflectedMemoryRange r) { return std::make_pair(r.offset, r.size); });
        // clang-format on

        apemode::CityHash64Wrapper city64 = {};
        city64.CombineWithArray(&*reflectedResource.ActiveRanges.begin(), &*reflectedResource.ActiveRanges.end());
        city64.CombineWith(reflectedState.bIsActive);
        
        reflectedState.Hash = city64.Value;
        return reflectedState;
    }
    uint32_t GetReflectedResourceStateIndex(const HashedReflectedResourceState& reflected) {
        auto it = std::find_if(uniqueReflectedResourceStates.begin(), uniqueReflectedResourceStates.end(), [reflected](const HashedReflectedResourceState& existing) { return existing.Hash == reflected.Hash; });
        if (it != uniqueReflectedResourceStates.end()) { return std::distance(uniqueReflectedResourceStates.begin(), it); }
        uint32_t index = uniqueReflectedResourceStates.size();
        uniqueReflectedResourceStates.push_back(reflected);
        return index;
    }
    HashedReflectedType GetHashedReflectedType(const apemode::shp::ReflectedType& reflectedType) {
        HashedReflectedType hashedReflectedType = {};
        hashedReflectedType.NameIndex = GetStringIndex(reflectedType.Name);
        hashedReflectedType.ElementPrimitiveType = cso::ReflectedPrimitiveType(reflectedType.ElementPrimitiveType);
        hashedReflectedType.ElementByteSize = reflectedType.ElementByteSize;
        hashedReflectedType.ElementVectorLength = reflectedType.ElementVectorLength;
        hashedReflectedType.ElementColumnCount = reflectedType.ElementColumnCount;
        hashedReflectedType.ElementMatrixByteStride = reflectedType.ElementMatrixByteStride;
        hashedReflectedType.ArrayLength = reflectedType.ArrayLength;
        hashedReflectedType.bIsArrayLengthStatic = reflectedType.bIsArrayLengthStatic;
        hashedReflectedType.ArrayByteStride = reflectedType.ArrayByteStride;
        hashedReflectedType.EffectiveByteSize = reflectedType.EffectiveByteSize;
        
        for (auto& reflected_member_type : reflectedType.Members) {
            HashedReflectedType memberHashedType = GetHashedReflectedType(reflected_member_type->Type);
            
            HashedReflectedTypeMember hashedReflectedMemberType = {};
            hashedReflectedMemberType.NameIndex = GetStringIndex(reflected_member_type->Name);
            hashedReflectedMemberType.EffectiveByteSize = reflected_member_type->EffectiveByteSize;
            hashedReflectedMemberType.OccupiedByteSize = reflected_member_type->OccupiedByteSize;
            hashedReflectedMemberType.ByteOffset = reflected_member_type->ByteOffset;
            hashedReflectedMemberType.TypeIndex = GetReflectedTypeIndex(memberHashedType);
            hashedReflectedType.MemberTypes.push_back(hashedReflectedMemberType);
        }

        apemode::CityHash64Wrapper city64 = {};
        city64.CombineWith(hashedReflectedType.NameIndex);
        city64.CombineWith(hashedReflectedType.ElementPrimitiveType);
        city64.CombineWith(hashedReflectedType.ElementByteSize);
        city64.CombineWith(hashedReflectedType.ElementVectorLength);
        city64.CombineWith(hashedReflectedType.ElementColumnCount);
        city64.CombineWith(hashedReflectedType.ElementMatrixByteStride);
        city64.CombineWith(hashedReflectedType.ArrayLength);
        city64.CombineWith(hashedReflectedType.bIsArrayLengthStatic);
        city64.CombineWith(hashedReflectedType.ArrayByteStride);
        city64.CombineWith(hashedReflectedType.EffectiveByteSize);

        for (const auto& members : hashedReflectedType.MemberTypes) {
            city64.CombineWith(members.NameIndex);
            city64.CombineWith(members.TypeIndex);
            city64.CombineWith(members.ByteOffset);
            city64.CombineWith(members.EffectiveByteSize);
            city64.CombineWith(members.OccupiedByteSize);
        }

        hashedReflectedType.Hash = city64.Value;
        return hashedReflectedType;
    }
    uint32_t GetReflectedTypeIndex(const HashedReflectedType& reflectedType) {
        auto it = std::find_if(uniqueReflectedTypes.begin(), uniqueReflectedTypes.end(), [reflectedType](const HashedReflectedType& existingReflectedType) { return existingReflectedType.Hash == reflectedType.Hash; });
        if (it != uniqueReflectedTypes.end()) { return std::distance(uniqueReflectedTypes.begin(), it); }
        uint32_t index = uniqueReflectedTypes.size();
        uniqueReflectedTypes.push_back(reflectedType);
        return index;
    }
    HashedReflectedResource GetHashedReflectedResource(const apemode::shp::ReflectedResource& reflectedResource) {
        HashedReflectedType hashedType = GetHashedReflectedType(reflectedResource.Type);
        
        HashedReflectedResource hashedReflectedResource = {};
        hashedReflectedResource.NameIndex = GetStringIndex(reflectedResource.Name);
        hashedReflectedResource.TypeIndex = GetReflectedTypeIndex(hashedType);
        hashedReflectedResource.DescriptorSet = reflectedResource.DecorationDescriptorSet;
        hashedReflectedResource.DescriptorBinding = reflectedResource.DecorationBinding;
        hashedReflectedResource.Locaton = reflectedResource.DecorationLocation;
        
        apemode::CityHash64Wrapper city64 = {};
        city64.CombineWith(hashedReflectedResource.NameIndex);
        city64.CombineWith(hashedReflectedResource.TypeIndex);
        city64.CombineWith(hashedReflectedResource.DescriptorSet);
        city64.CombineWith(hashedReflectedResource.DescriptorBinding);
        city64.CombineWith(hashedReflectedResource.Locaton);
        
        hashedReflectedResource.Hash = city64.Value;
        return hashedReflectedResource;
    }
    uint32_t GetReflectedResourceIndex(const HashedReflectedResource& reflectedResource) {
        auto it = std::find_if(uniqueReflectedResources.begin(), uniqueReflectedResources.end(), [reflectedResource](const HashedReflectedResource& existingReflectedResource) { return existingReflectedResource.Hash == reflectedResource.Hash; });
        if (it != uniqueReflectedResources.end()) { return std::distance(uniqueReflectedResources.begin(), it); }
        uint32_t index = uniqueReflectedResources.size();
        uniqueReflectedResources.push_back(reflectedResource);
        return index;
    }
    HashedReflectedConstant GetHashedReflectedConstant(const apemode::shp::ReflectedConstant& reflectedConstant) {
        HashedReflectedType hashedType = GetHashedReflectedType(reflectedConstant.Type);
        
        HashedReflectedConstant hashedReflectedConstant = {};
        hashedReflectedConstant.NameIndex = GetStringIndex(reflectedConstant.Name);
        hashedReflectedConstant.MacroIndex = GetStringIndex(reflectedConstant.MacroName);
        hashedReflectedConstant.TypeIndex = GetReflectedTypeIndex(hashedType);
        hashedReflectedConstant.ConstantId = reflectedConstant.ConstantId;
        hashedReflectedConstant.DefaultScalarU64 = reflectedConstant.DefaultValue.u64;
        hashedReflectedConstant.bIsSpecialization = reflectedConstant.bIsSpecialization;
        hashedReflectedConstant.bIsUsedAsArrayLength = reflectedConstant.bIsUsedAsArrayLength;
        hashedReflectedConstant.bIsUsedAsLUT = reflectedConstant.bIsUsedAsLUT;
        
        apemode::CityHash64Wrapper city64 = {};
        city64.CombineWith(hashedReflectedConstant.NameIndex);
        city64.CombineWith(hashedReflectedConstant.MacroIndex);
        city64.CombineWith(hashedReflectedConstant.TypeIndex);
        city64.CombineWith(hashedReflectedConstant.ConstantId);
        city64.CombineWith(hashedReflectedConstant.DefaultScalarU64);
        city64.CombineWith(hashedReflectedConstant.bIsSpecialization);
        city64.CombineWith(hashedReflectedConstant.bIsUsedAsArrayLength);
        city64.CombineWith(hashedReflectedConstant.bIsUsedAsLUT);
        
        hashedReflectedConstant.Hash = city64.Value;
        return hashedReflectedConstant;
    }
    uint32_t GetReflectedConstantIndex(const HashedReflectedConstant& reflectedConstant) {
        auto it = std::find_if(uniqueReflectedConstants.begin(), uniqueReflectedConstants.end(), [reflectedConstant](const HashedReflectedConstant& existingReflectedConstant) { return existingReflectedConstant.Hash == reflectedConstant.Hash; });
        if (it != uniqueReflectedConstants.end()) { return std::distance(uniqueReflectedConstants.begin(), it); }
        uint32_t index = uniqueReflectedConstants.size();
        uniqueReflectedConstants.push_back(reflectedConstant);
        return index;
    }
    HashedReflectedShader GetHashedReflectionShader(const apemode::shp::ReflectedShader& reflectedShader) {
        HashedReflectedShader hashedReflectedShader = {};
        apemode::CityHash64Wrapper city64 = {};
        
        hashedReflectedShader.NameIndex = GetStringIndex(reflectedShader.Name);
        city64.CombineWith(hashedReflectedShader.NameIndex);
        
        for (auto& reflectedConstant : reflectedShader.Constants) {
            HashedReflectedConstant hashedConstant = GetHashedReflectedConstant(reflectedConstant);
            hashedReflectedShader.ConstantIndices.push_back(GetReflectedConstantIndex(hashedConstant));
            city64.CombineWith(hashedConstant.Hash);
        }
        for (auto& reflectedResource : reflectedShader.StageInputs) {
            HashedReflectedResource hashedResource = GetHashedReflectedResource(reflectedResource);
            hashedReflectedShader.StageInputIndices.push_back(GetReflectedResourceIndex(hashedResource));
            city64.CombineWith(hashedResource.Hash);
        }
        for (auto& reflectedResource : reflectedShader.StageOutputs) {
            HashedReflectedResource hashedResource = GetHashedReflectedResource(reflectedResource);
            hashedReflectedShader.StageOutputIndices.push_back(GetReflectedResourceIndex(hashedResource));
            city64.CombineWith(hashedResource.Hash);
        }
        for (auto& reflectedResource : reflectedShader.UniformBuffers) {
            HashedReflectedResource hashedResource = GetHashedReflectedResource(reflectedResource);
            hashedReflectedShader.UniformBufferIndices.push_back(GetReflectedResourceIndex(hashedResource));
            city64.CombineWith(hashedResource.Hash);
            
            HashedReflectedResourceState state = GetHashedReflectedResourceState(reflectedResource);
            hashedReflectedShader.UniformBufferStateIndices.push_back(GetReflectedResourceStateIndex(state));
            city64.CombineWith(hashedResource.Hash);
        }
        for (auto& reflectedResource : reflectedShader.PushConstantBuffers) {
            HashedReflectedResource hashedResource = GetHashedReflectedResource(reflectedResource);
            hashedReflectedShader.PushConstantBufferIndices.push_back(GetReflectedResourceIndex(hashedResource));
            city64.CombineWith(hashedResource.Hash);
            
            HashedReflectedResourceState state = GetHashedReflectedResourceState(reflectedResource);
            hashedReflectedShader.UniformBufferStateIndices.push_back(GetReflectedResourceStateIndex(state));
            city64.CombineWith(hashedResource.Hash);
        }
        for (auto& reflectedResource : reflectedShader.SampledImages) {
            HashedReflectedResource hashedResource = GetHashedReflectedResource(reflectedResource);
            hashedReflectedShader.SampledImageIndices.push_back(GetReflectedResourceIndex(hashedResource));
            city64.CombineWith(hashedResource.Hash);
        }
        for (auto& reflectedResource : reflectedShader.SubpassInputs) {
            HashedReflectedResource hashedResource = GetHashedReflectedResource(reflectedResource);
            hashedReflectedShader.SubpassImageIndices.push_back(GetReflectedResourceIndex(hashedResource));
            city64.CombineWith(hashedResource.Hash);
        }
        for (auto& reflectedResource : reflectedShader.SeparateImages) {
            HashedReflectedResource hashedResource = GetHashedReflectedResource(reflectedResource);
            hashedReflectedShader.SeparateImageIndices.push_back(GetReflectedResourceIndex(hashedResource));
            city64.CombineWith(hashedResource.Hash);
        }
        for (auto& reflectedResource : reflectedShader.SeparateSamplers) {
            HashedReflectedResource hashedResource = GetHashedReflectedResource(reflectedResource);
            hashedReflectedShader.SeparateSamplerIndices.push_back(GetReflectedResourceIndex(hashedResource));
            city64.CombineWith(hashedResource.Hash);
        }
        for (auto& reflectedResource : reflectedShader.StorageImages) {
            HashedReflectedResource hashedResource = GetHashedReflectedResource(reflectedResource);
            hashedReflectedShader.StorageImageIndices.push_back(GetReflectedResourceIndex(hashedResource));
            city64.CombineWith(hashedResource.Hash);
        }
        for (auto& reflectedResource : reflectedShader.StorageBuffers) {
            HashedReflectedResource hashedResource = GetHashedReflectedResource(reflectedResource);
            hashedReflectedShader.StorageBufferIndices.push_back(GetReflectedResourceIndex(hashedResource));
            city64.CombineWith(hashedResource.Hash);
            
            HashedReflectedResourceState state = GetHashedReflectedResourceState(reflectedResource);
            hashedReflectedShader.UniformBufferStateIndices.push_back(GetReflectedResourceStateIndex(state));
            city64.CombineWith(hashedResource.Hash);
        }
        
        hashedReflectedShader.Hash = city64.Value;
        return hashedReflectedShader;
    }
    uint32_t GetReflectedShaderIndex(const HashedReflectedShader& reflectedShader) {
        auto it = std::find_if(uniqueReflectedShaders.begin(), uniqueReflectedShaders.end(), [reflectedShader](const HashedReflectedShader& existingReflectedShader) { return existingReflectedShader.Hash == reflectedShader.Hash; });
        if (it != uniqueReflectedShaders.end()) { return std::distance(uniqueReflectedShaders.begin(), it); }
        uint32_t index = uniqueReflectedShaders.size();
        uniqueReflectedShaders.push_back(reflectedShader);
        return index;
    }
    uint32_t GetCompiledShaderIndex(const HashedCompiledShader& compiledShader) {
        auto it = std::find_if(uniqueCompiledShaders.begin(), uniqueCompiledShaders.end(), [compiledShader](const HashedCompiledShader& existingCompiledShader) { return existingCompiledShader.Hash == compiledShader.Hash; });
        if (it != uniqueCompiledShaders.end()) { return std::distance(uniqueCompiledShaders.begin(), it); }
        uint32_t index = uniqueCompiledShaders.size();
        uniqueCompiledShaders.push_back(compiledShader);
        return index;
    }
    uint32_t GetStringIndex(const std::string& string) {
        uint64_t hash = apemode::CityHash64(string.data(), string.size());
        auto it = std::find_if(uniqueStrings.begin(), uniqueStrings.end(), [hash](const UniqueString& hashedString) { return hashedString.Hash == hash; });
        if (it != uniqueStrings.end()) { return std::distance(uniqueStrings.begin(), it); }
        uint32_t index = uniqueStrings.size();
        uniqueStrings.push_back({hash, string});
        return index;
    }
    uint32_t GetBufferIndex(const std::vector<uint8_t>& buffer) {
        uint64_t hash = apemode::CityHash64((const char*)buffer.data(), buffer.size());
        auto it = std::find_if(uniqueBuffers.begin(), uniqueBuffers.end(), [hash](const UniqueBuffer& hashedBuffer) { return hashedBuffer.Hash == hash; });
        if (it != uniqueBuffers.end()) { return std::distance(uniqueBuffers.begin(), it); }
        uint32_t index = uniqueBuffers.size();
        uniqueBuffers.push_back({hash, buffer});
        return index;
    }
    // clang-format on
};

class ShaderCompilerIncludedFileSet : public apemode::shp::IShaderCompiler::IIncludedFileSet {
public:
    std::set<std::string> IncludedFiles;
    void InsertIncludedFile(const std::string& includedFileName) override { IncludedFiles.insert(includedFileName); }
};

class ShaderCompilerMacroDefinitionCollection : public apemode::shp::IShaderCompiler::IMacroDefinitionCollection {
public:
    std::map<std::string, std::string> Macros;

    void Init(const std::map<std::string, std::string>& macros) { Macros = macros; }
    size_t GetCount() const override { return Macros.size(); }

    apemode::shp::IShaderCompiler::MacroDefinition GetMacroDefinition(const size_t macroIndex) const override {
        auto macroIt = Macros.begin();
        std::advance(macroIt, macroIndex);

        apemode::shp::IShaderCompiler::MacroDefinition macroDefinition;
        macroDefinition.pszKey = macroIt->first.c_str();
        macroDefinition.pszValue = macroIt->second.c_str();
        return macroDefinition;
    }
};

class ShaderFileReader : public apemode::shp::IShaderCompiler::IShaderFileReader {
public:
    apemode::platform::IAssetManager* mAssetManager;

    bool ReadShaderTxtFile(const std::string& InFilePath, std::string& OutFileFullPath, std::string& OutFileContent) {
        if (auto pAsset = mAssetManager->Acquire(InFilePath.c_str())) {
            const auto assetText = pAsset->GetContentAsTextBuffer();
            OutFileContent = reinterpret_cast<const char*>(assetText.data());
            OutFileFullPath = pAsset->GetId();
            mAssetManager->Release(pAsset);
            return true;
        }

        // apemodevk::platform::DebugBreak( );
        return false;
    }
};

class ShaderFeedbackWriter : public apemode::shp::IShaderCompiler::IShaderFeedbackWriter {
public:
    void WriteFeedback(EFeedbackType eType,
                       const std::string& FullFilePath,
                       const apemode::shp::IShaderCompiler::IMacroDefinitionCollection* pMacros,
                       const void* pContent,
                       const void* pContentEnd) {
        const auto feedbackStage = eType & eFeedbackType_CompilationStageMask;
        const auto feedbackCompilationError = eType & eFeedbackType_CompilationStatusMask;

        if (eFeedbackType_CompilationStatus_Success != feedbackCompilationError) {
            apemode::LogError("ShaderCompiler: {}/{}: {}",
                              feedbackStage,
                              feedbackCompilationError,
                              FullFilePath);
            apemode::LogError(" Msg: {}", (const char*)pContent);
            assert(false);
        } else {
            // apemode::LogInfo("ShaderCompiler: {}/{}: {}",
            //                  EFeedbackTypeWithOStream(feedbackStage),
            //                  EFeedbackTypeWithOStream(feedbackCompilationError),
            //                  FullFilePath);
        }
    }
};

class ShaderCompilerMacroGroupCollection {
public:
    std::vector<ShaderCompilerMacroDefinitionCollection> MacroGroups;

    void Init(const std::vector<ShaderCompilerMacroDefinitionCollection>& groups) { MacroGroups = groups; }
    size_t GetCount() const { return MacroGroups.size(); }

    const apemode::shp::IShaderCompiler::IMacroDefinitionCollection* GetMacroGroup(const size_t groupIndex) const {
        return &MacroGroups[groupIndex];
    }
};

void AssertContainsWhitespace(const std::string& s) {
    bool containsWhitespace = false;
    for (auto c : s) { containsWhitespace |= isspace(c); }

    assert(!containsWhitespace);
    if (containsWhitespace) { apemode::LogError("Should not contain whitespace, '{}'", s); }
}

std::string GetMacrosString(const std::map<std::string, std::string>& macros) {
    if (macros.empty()) { return ""; }

    std::string macrosString;
    for (const auto& macro : macros) {
        AssertContainsWhitespace(macro.first);
        macrosString += macro.first;
        macrosString += "=";
        macrosString += macro.second;
        macrosString += ";";
    }

    macrosString.erase(macrosString.size() - 1);
    return macrosString;
}

void ReplaceAll(std::string& data, const std::string& toSearch, const std::string& replaceStr) {
    size_t pos = data.find(toSearch);
    while (pos != std::string::npos) {
        data.replace(pos, toSearch.size(), replaceStr);
        pos = data.find(toSearch, pos + replaceStr.size());
    }
}

apemode::shp::IShaderCompiler::ShaderType GetShaderType(const std::string& shaderType) {
    if (shaderType == "vert") {
        return apemode::shp::IShaderCompiler::ShaderType::Vertex;
    } else if (shaderType == "frag") {
        return apemode::shp::IShaderCompiler::ShaderType::Fragment;
    } else if (shaderType == "comp") {
        return apemode::shp::IShaderCompiler::ShaderType::Compute;
    } else if (shaderType == "geom") {
        return apemode::shp::IShaderCompiler::ShaderType::Geometry;
    } else if (shaderType == "tesc") {
        return apemode::shp::IShaderCompiler::ShaderType::TessControl;
    } else if (shaderType == "tese") {
        return apemode::shp::IShaderCompiler::ShaderType::TessEvaluation;
    } else if (shaderType == "rgen") {
        return apemode::shp::IShaderCompiler::ShaderType::RayGeneration;
    } else if (shaderType == "ahit") {
        return apemode::shp::IShaderCompiler::ShaderType::AnyHit;
    } else if (shaderType == "chit") {
        return apemode::shp::IShaderCompiler::ShaderType::ClosestHit;
    } else if (shaderType == "miss") {
        return apemode::shp::IShaderCompiler::ShaderType::Miss;
    } else if (shaderType == "call") {
        return apemode::shp::IShaderCompiler::ShaderType::Callable;
    } else if (shaderType == "mesh") {
        return apemode::shp::IShaderCompiler::ShaderType::Mesh;
    } else if (shaderType == "task") {
        return apemode::shp::IShaderCompiler::ShaderType::Task;
    }

    apemode::LogError("Caught unexpected shader type: {}", shaderType);
    return apemode::shp::IShaderCompiler::ShaderType::Count;
}

std::map<std::string, std::string> GetMacroDefinitions(const json& macrosJson) {
    std::map<std::string, std::string> macroDefinitions;
    for (const auto& macroJson : macrosJson) {
        assert(macroJson.is_object());
        assert(macroJson["name"].is_string());

        const std::string macroName = macroJson["name"].get<std::string>();
        macroDefinitions[macroName] = "1";

        auto valueJsonIt = macroJson.find("value");
        if (valueJsonIt != macroJson.end() && valueJsonIt->is_boolean()) {
            if (false == valueJsonIt->get<bool>()) { macroDefinitions[macroName] = "0"; }
        } else if (valueJsonIt->is_number_integer()) {
            macroDefinitions[macroName] = std::to_string(valueJsonIt->get<int>());
        } else if (valueJsonIt->is_number_unsigned()) {
            macroDefinitions[macroName] = std::to_string(valueJsonIt->get<unsigned>());
        } else if (valueJsonIt->is_number_float()) {
            macroDefinitions[macroName] = std::to_string(valueJsonIt->get<int>());
        }
    }

    return macroDefinitions;
}

ShaderCompilerMacroDefinitionCollection GetMacroGroup(const json& groupJson) {
    std::map<std::string, std::string> macroDefinitions;
    for (const auto& macroJson : groupJson) {
        assert(macroJson.is_object());
        assert(macroJson["name"].is_string());

        const std::string macroName = macroJson["name"].get<std::string>();
        macroDefinitions[macroName] = "1";

        auto valueJsonIt = macroJson.find("value");
        if (valueJsonIt != macroJson.end()) {
            if (valueJsonIt->is_boolean()) {
                if (false == valueJsonIt->get<bool>()) { macroDefinitions[macroName] = "0"; }
            } else if (valueJsonIt->is_number_integer()) {
                macroDefinitions[macroName] = std::to_string(valueJsonIt->get<int>());
            } else if (valueJsonIt->is_number_unsigned()) {
                macroDefinitions[macroName] = std::to_string(valueJsonIt->get<unsigned>());
            } else if (valueJsonIt->is_number_float()) {
                macroDefinitions[macroName] = std::to_string(valueJsonIt->get<int>());
            }
        }
    }

    ShaderCompilerMacroDefinitionCollection group;
    group.Init(macroDefinitions);
    return group;
}

std::optional<CompiledShaderVariant> CompileShaderVariant(const apemode::shp::IShaderCompiler& shaderCompiler,
                                                          const std::map<std::string, std::string>& macroDefinitions,
                                                          const std::string& shaderType,
                                                          const std::string& srcFile,
                                                          const std::string& outputFolder) {
    std::string macrosString = GetMacrosString(macroDefinitions);
    apemode::LogInfo("Variant: asset=\"{}\", definitions=\"{}\"", srcFile, macrosString);

    CompiledShaderVariant cso = {};
    cso.Definitions = macrosString;

    ShaderCompilerMacroDefinitionCollection concreteMacros;
    concreteMacros.Init(macroDefinitions);

    apemode::shp::IShaderCompiler::ShaderType eShaderType = GetShaderType(shaderType);
    if (eShaderType == apemode::shp::IShaderCompiler::ShaderType::Count) { return {}; }
    
    cso.Type = cso::Shader(eShaderType);

    ShaderCompilerIncludedFileSet includedFileSet;
    if (auto compiledShader = shaderCompiler.Compile(srcFile,
                                                     &concreteMacros,
                                                     eShaderType,
                                                     apemode::shp::IShaderCompiler::eShaderOptimization_None,
                                                     &includedFileSet)) {
        cso.Compiled.assign(compiledShader->GetBytePtr(),
                            compiledShader->GetBytePtr() + compiledShader->GetByteCount());
        cso.Preprocessed = compiledShader->GetPreprocessedSrc();
        cso.Assembly = compiledShader->GetAssemblySrc();
        cso.CompiledGLSL = compiledShader->GetCompiledGLSL();
        cso.CompiledMSL = compiledShader->GetCompiledMSL();
        cso.Asset = srcFile;
        cso.IncludedFiles = includedFileSet.IncludedFiles;
        cso.DefinitionMap = macroDefinitions;
        cso.Definitions = macrosString;
        cso.Reflected = std::move(compiledShader->GetReflection());
        cso.Type = cso::Shader(eShaderType);

        ReplaceAll(macrosString, ".", "-");
        ReplaceAll(macrosString, ";", "+");

        std::string dstFile = srcFile;
        const size_t fileStartPos = dstFile.find_last_of("/\\");
        if (fileStartPos != dstFile.npos) { dstFile = dstFile.substr(fileStartPos + 1); }

        std::string dstFilePath =
            outputFolder + "/" + dstFile + (macrosString.empty() ? "" : "-defs-") + macrosString + ".spv";

        ReplaceAll(dstFilePath, "//", "/");
        ReplaceAll(dstFilePath, "\\/", "\\");
        ReplaceAll(dstFilePath, "\\\\", "\\");

        std::string cachedPreprocessed = dstFilePath + "-preprocessed.txt";
        std::string cachedAssembly = dstFilePath + "-assembly.txt";
        std::string cachedGLSL = dstFilePath + "-c-glsl.txt";
        std::string cachedMSL = dstFilePath + "-c-msl.txt";

        flatbuffers::SaveFile(
            dstFilePath.c_str(), (const char*)compiledShader->GetBytePtr(), compiledShader->GetByteCount(), true);

        // clang-format off
        flatbuffers::SaveFile(cachedPreprocessed.c_str(),
                              compiledShader->GetPreprocessedSrc().data(),
                              compiledShader->GetPreprocessedSrc().size(),
                              false);

        flatbuffers::SaveFile(cachedAssembly.c_str(),
                              compiledShader->GetAssemblySrc().data(),
                              compiledShader->GetAssemblySrc().size(),
                              false);

        flatbuffers::SaveFile(cachedGLSL.c_str(),
                              compiledShader->GetCompiledGLSL().data(),
                              compiledShader->GetCompiledGLSL().size(),
                              false);

        flatbuffers::SaveFile(cachedMSL.c_str(),
                              compiledShader->GetCompiledMSL().data(),
                              compiledShader->GetCompiledMSL().size(),
                              false);
        // clang-format on

        return cso;
    }

    return {};
}

void CompileShaderVariantsRecursively(std::vector<CompiledShaderVariant>& csos,
                                      const apemode::shp::IShaderCompiler& shaderCompiler,
                                      const ShaderCompilerMacroGroupCollection& macroGroups,
                                      const size_t macroGroupIndex,
                                      const std::string& shaderType,
                                      const std::string& srcFile,
                                      const std::string& outputFolder,
                                      std::vector<size_t>& macroIndices) {
    assert(macroIndices.size() == macroGroups.MacroGroups.size());
    if (macroGroupIndex >= macroGroups.MacroGroups.size()) {
        std::map<std::string, std::string> macroDefinitions;

        // apemode::LogInfo("Collecting definitions ...");
        for (size_t i = 0; i < macroGroups.GetCount(); ++i) {
            auto g = macroGroups.GetMacroGroup(i);
            auto j = macroIndices[i];
            auto k = g->GetMacroDefinition(j).pszKey;
            auto v = g->GetMacroDefinition(j).pszValue;

            if (strcmp(k, "") != 0) {
                // apemode::LogInfo("Adding: group {}, macro {}, {}={}", i, j, k, v);
                macroDefinitions[k] = v;
            } else {
                // apemode::LogInfo("Skipping: group={}", i);
            }
        }

        // apemode::LogInfo("Sending to compiler ...");

        if (auto cso = CompileShaderVariant(shaderCompiler, macroDefinitions, shaderType, srcFile, outputFolder)) {
            csos.emplace_back(std::move(cso).value());
        }
        return;
    }

    auto& group = macroGroups.MacroGroups[macroGroupIndex];
    for (size_t i = 0; i < group.GetCount(); ++i) {
        macroIndices[macroGroupIndex] = i;
        CompileShaderVariantsRecursively(
            csos, shaderCompiler, macroGroups, macroGroupIndex + 1, shaderType, srcFile, outputFolder, macroIndices);
    }
}

void CompileShaderVariantsRecursively(std::vector<CompiledShaderVariant>& csos,
                                      const apemode::shp::IShaderCompiler& shaderCompiler,
                                      const ShaderCompilerMacroGroupCollection& macroGroups,
                                      const std::string& shaderType,
                                      const std::string& srcFile,
                                      const std::string& outputFolder) {
    std::vector<size_t> macroIndices(macroGroups.GetCount());
    CompileShaderVariantsRecursively(
        csos, shaderCompiler, macroGroups, 0, shaderType, srcFile, outputFolder, macroIndices);
}

std::vector<CompiledShaderVariant> CompileShader(const apemode::platform::shared::AssetManager& assetManager,
                                                 apemode::shp::IShaderCompiler& shaderCompiler,
                                                 const json& commandJson,
                                                 const std::string& outputFolder) {
    std::vector<CompiledShaderVariant> csos;

    assert(commandJson["srcFile"].is_string());
    std::string srcFile = commandJson["srcFile"].get<std::string>();
    // apemode::LogInfo("srcFile: {}", srcFile.c_str());

    if (auto acquiredSrcFile = assetManager.Acquire(srcFile.c_str())) {
        // apemode::LogInfo("assetId: {}", acquiredSrcFile->GetId());

        auto definitionGroupsJsonIt = commandJson.find("definitionGroups");
        if (definitionGroupsJsonIt != commandJson.end() && definitionGroupsJsonIt->is_array()) {
            ShaderCompilerMacroGroupCollection macroGroups;

            const json& definitionGroupsJson = *definitionGroupsJsonIt;
            for (auto& definitionGroupJson : definitionGroupsJson) {
                macroGroups.MacroGroups.push_back(GetMacroGroup(definitionGroupJson));
            }

            assert(commandJson["shaderType"].is_string());
            const std::string shaderType = commandJson["shaderType"].get<std::string>();

            CompileShaderVariantsRecursively(csos, shaderCompiler, macroGroups, shaderType, srcFile, outputFolder);
        } else {
            std::map<std::string, std::string> macroDefinitions;
            ShaderCompilerMacroDefinitionCollection macros;

            auto macrosJsonIt = commandJson.find("macros");
            if (macrosJsonIt != commandJson.end() && macrosJsonIt->is_array()) {
                const json& macrosJson = *macrosJsonIt;
                macroDefinitions = GetMacroDefinitions(macrosJson);

                if (!macroDefinitions.empty()) { macros.Init(macroDefinitions); }
            }

            assert(commandJson["shaderType"].is_string());
            const std::string shaderType = commandJson["shaderType"].get<std::string>();

            if (auto cso = CompileShaderVariant(shaderCompiler, macroDefinitions, shaderType, srcFile, outputFolder)) {
                csos.emplace_back(std::move(cso).value());
            }
        }

        return csos;
    }

    apemode::LogError("Asset not found, the command \"{}\" skipped.", commandJson.dump().c_str());
    return {};
}
} // namespace


template < int TPrecision = 100 >
float RoundOff( float n ) {
    const float i = n * static_cast< float >( TPrecision ) + 0.5f;
    return ( (float) (int) i ) / static_cast< float >( TPrecision );
}

// clang-format off
std::string ToPrettySizeString(size_t size) {
    constexpr std::array<std::string_view, 4> kSizeStrings = {"B", "KB", "MB", "GB"};
    size_t div = 0;
    size_t rem = 0;
    while (size >= 1024 && div < kSizeStrings.size()) { rem = size % 1024 ; div++; size /= 1024; }
    const float size_d = static_cast<float>(size) +  static_cast<float>( rem ) / 1024.0f;
    std::string string = std::to_string(RoundOff(size_d));
    string += kSizeStrings[div];
    return string;
}
// clang-format on

int main(int argc, char** argv) {
    /* Input parameters:
     * --assets-folder "/Users/vlad.serhiienko/Projects/Home/Viewer/assets"
     * --cso-json-file "/Users/vlad.serhiienko/Projects/Home/Viewer/assets/shaders/Viewer.cso.json"
     * --cso-file "/Users/vlad.serhiienko/Projects/Home/Viewer/assets/shaders/Viewer.cso"
     */
    apemode::AppState::OnMain(argc, (const char**)argv);
    apemode::AppStateExitGuard eg{};

    std::string assetsFolderWildcard = apemode::TGetOption("assets-folder", std::string());
    if (assetsFolderWildcard.empty()) {
        apemode::LogError("No assets folder.");
        return 1;
    }

    std::string outputFile = apemode::TGetOption("cso-file", std::string());
    if (outputFile.empty()) {
        apemode::LogError("Output CSO file is empty.");
        return 1;
    }

    std::string outputFolder = outputFile + ".d";
#if defined(_WIN32)
    CreateDirectory(outputFolder.c_str(), NULL);
#else
    mkdir(outputFolder.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
#endif

    if (outputFile.empty()) {
        apemode::LogError("Output CSO file is empty.");
        return 1;
    }

    std::string assetsFolder = assetsFolderWildcard;
    ReplaceAll(assetsFolder, "*", "");
    ReplaceAll(assetsFolder, "\\", "/");
    ReplaceAll(assetsFolder, "//", "/");

    apemode::LogInfo("Asset folder: {}", assetsFolder);
    apemode::LogInfo("Asset folder (wildcard): {}", assetsFolderWildcard);

    apemode::platform::shared::AssetManager assetManager;
    assetManager.UpdateAssets(assetsFolderWildcard.c_str(), nullptr, 0);

    std::string csoJsonFile = apemode::TGetOption("cso-json-file", std::string());
    if (csoJsonFile.empty() || !apemode::platform::shared::FileExists(csoJsonFile.c_str())) {
        apemode::LogError("No CSO file.");
        return 1;
    }

    apemode::LogInfo("CSO JSON file: {}", csoJsonFile);

    auto csoJsonContents = apemode::platform::shared::FileReader().ReadTxtFile(csoJsonFile.c_str());
    if (csoJsonFile.empty()) {
        apemode::LogError("CSO file is empty.");
        return 1;
    }

    const json csoJson = json::parse((const char*)csoJsonContents.data());
    if (!csoJson.is_object()) {
        apemode::LogError("Parsing error.");
        return 1;
    }

    assert(csoJson["commands"].is_array());
    if (!csoJson["commands"].is_array()) {
        apemode::LogError("Parsing error.");
        return 1;
    }

    auto shaderCompiler = apemode::shp::NewShaderCompiler();

    ShaderFileReader shaderCompilerFileReader;
    ShaderFeedbackWriter shaderFeedbackWriter;

    shaderCompilerFileReader.mAssetManager = &assetManager;
    shaderCompiler->SetShaderFileReader(&shaderCompilerFileReader);
    shaderCompiler->SetShaderFeedbackWriter(&shaderFeedbackWriter);

    std::vector<CompiledShaderVariant> compiledShaders;

    // clang-format off
    const json& commandsJson = csoJson["commands"];
    for (const auto& commandJson : commandsJson) {
        std::vector<CompiledShaderVariant> variants = CompileShader(assetManager, *shaderCompiler, commandJson, outputFolder);
        compiledShaders.insert(compiledShaders.end(), std::make_move_iterator(variants.begin()), std::make_move_iterator(variants.end()));
    }
    // clang-format on

    CompiledShaderCollection collection;
    flatbuffers::FlatBufferBuilder fbb;
    collection.Serialize(fbb, compiledShaders);

    flatbuffers::Verifier v(fbb.GetBufferPointer(), fbb.GetSize());
    assert(cso::VerifyCompiledShaderCollectionBuffer(v));

    auto builtBuffePtr = (const char*)fbb.GetBufferPointer();
    auto builtBuffeLen = (size_t)fbb.GetSize();
    
    apemode::LogInfo("= {} bytes = ~{}", builtBuffeLen, ToPrettySizeString(builtBuffeLen));

    apemode::LogInfo("CSO file: {}", outputFile);
    if (!flatbuffers::SaveFile(outputFile.c_str(), builtBuffePtr, builtBuffeLen, true)) {
        apemode::LogError("Failed to write CSO ({} bytes) to file: '{}'", builtBuffeLen, outputFile);
        return 1;
    }

    apemode::LogInfo("Done.");
    return 0;
}
