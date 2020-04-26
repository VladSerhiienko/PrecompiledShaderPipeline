
#include <apemode/platform/AppState.h>
#include <apemode/platform/CityHash.h>
#include <flatbuffers/util.h>

#include <cstdint>
#include <filesystem>
#include <iterator>
#include <memory>
#include <nlohmann/json.hpp>

#include "ShaderCompiler.h"
#include "cso_generated.h"

using json = nlohmann::json;

std::string ReadTextFile(const std::string& filePath) {
    std::string assetContentBuffer;

    FILE* pFile = fopen(filePath.c_str(), "rb");
    if (!pFile) return {};

    fseek(pFile, 0, SEEK_END);
    const size_t size = static_cast<size_t>(ftell(pFile));
    fseek(pFile, 0, SEEK_SET);

    assetContentBuffer.resize(size);
    if (assetContentBuffer.empty()) return {};

    fread(assetContentBuffer.data(), size, 1, pFile);
    fclose(pFile);

    return assetContentBuffer;
}

namespace {
struct CompiledShaderVariant {
    CompiledShaderVariant() = default;
    CompiledShaderVariant(CompiledShaderVariant&&) noexcept = default;
    CompiledShaderVariant& operator=(CompiledShaderVariant&&) = default;
    ~CompiledShaderVariant() = default;

    std::string Preprocessed = "";
    std::string Assembly = "";
    std::string Asset = "";
    std::string Definitions = "";
    std::string Vulkan = "";
    std::string iOS = "";
    std::string macOS = "";
    std::string ES2 = "";
    std::string ES3 = "";
    std::vector<uint8_t> Buffer = {};
    apemode::shp::ReflectedShader Reflected = {};
    cso::Shader Type = cso::Shader::Shader_MAX;
    std::set<std::string> IncludedFiles = {};
    std::map<std::string, std::string> DefinitionMap = {};
};

struct Hashed {
    uint64_t Hash = 0;
};

struct UniqueString : Hashed {
    std::string Contents = "";
};

struct UniqueBuffer : Hashed {
    std::vector<uint8_t> Contents = {};
};

struct HashedCompiledShader : Hashed {
    uint32_t BufferIndex = 0;
    uint32_t PreprocessedIndex = 0;
    uint32_t AssemblyIndex = 0;
    uint32_t VulkanIndex = 0;
    uint32_t iOSIndex = 0;
    uint32_t macOSIndex = 0;
    uint32_t ES2Index = 0;
    uint32_t ES3Index = 0;
    uint32_t ReflectedIndex = 0;
};

struct HashedCompiledShaderInfo : Hashed {
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

struct HashedReflectedType : Hashed {
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

struct HashedReflectedResourceState : Hashed {
    bool bIsActive = false;
    std::vector<std::pair<uint32_t, uint32_t>> ActiveRanges = {};
};

struct HashedReflectedResource : Hashed {
    uint32_t NameIndex = 0;
    uint32_t TypeIndex = 0;
    uint32_t DescriptorSet = 0;
    uint32_t DescriptorBinding = 0;
    uint32_t Locaton = 0;
};

struct HashedReflectedConstant : Hashed {
    uint32_t NameIndex = 0;
    uint32_t MacroIndex = 0;
    uint64_t DefaultScalarU64 = 0;
    uint32_t ConstantId = 0;
    uint32_t TypeIndex = 0;
    bool bIsSpecialization = false;
    bool bIsUsedAsArrayLength = false;
    bool bIsUsedAsLUT = false;
};

struct HashedReflectedShader : Hashed {
    uint32_t NameIndex = 0;
    std::vector<uint32_t> ConstantIndices = {};
    std::vector<uint32_t> StageInputIndices = {};
    std::vector<uint32_t> StageOutputIndices = {};
    std::vector<uint32_t> UniformBufferIndices = {};
    std::vector<uint32_t> PushConstantBufferIndices = {};
    std::vector<uint32_t> SampledImageIndices = {};
    std::vector<uint32_t> SubpassInputIndices = {};
    std::vector<uint32_t> SeparateImageIndices = {};
    std::vector<uint32_t> SeparateSamplerIndices = {};
    std::vector<uint32_t> StorageImageIndices = {};
    std::vector<uint32_t> StorageBufferIndices = {};

    std::vector<uint32_t> StageInputStateIndices = {};
    std::vector<uint32_t> StageOutputStateIndices = {};
    std::vector<uint32_t> UniformBufferStateIndices = {};
    std::vector<uint32_t> PushConstantBufferStateIndices = {};
    std::vector<uint32_t> SampledImageStateIndices = {};
    std::vector<uint32_t> SubpassInputStateIndices = {};
    std::vector<uint32_t> SeparateImageStateIndices = {};
    std::vector<uint32_t> SeparateSamplerStateIndices = {};
    std::vector<uint32_t> StorageImageStateIndices = {};
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

    void Serialize(flatbuffers::FlatBufferBuilder& fbb,
                   const std::vector<std::unique_ptr<CompiledShaderVariant>>& variants) {
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

            uint32_t arrayLength = reflectedType.ArrayLength & cso::ArrayLength_ValueBitMask;
            arrayLength |= reflectedType.bIsArrayLengthStatic ? cso::ArrayLength_IsStaticBitMask : 0;

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

            // clang-format off
            reflectedConstants.push_back(cso::ReflectedConstant(c.NameIndex, c.MacroIndex, c.DefaultScalarU64, c.ConstantId, c.TypeIndex, bits));
            // clang-format on
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
            auto subpassInputIndicesOffset = fbb.CreateVector(s.SubpassInputIndices);
            auto separateImageIndicesOffset = fbb.CreateVector(s.SeparateImageIndices);
            auto separateSamplerIndicesOffset = fbb.CreateVector(s.SeparateSamplerIndices);
            auto storageImageIndicesOffset = fbb.CreateVector(s.StorageImageIndices);
            auto storageBufferIndicesOffset = fbb.CreateVector(s.StorageBufferIndices);

            auto stageInputStateIndicesOffset = fbb.CreateVector(s.StageInputStateIndices);
            auto stageOutputStateIndicesOffset = fbb.CreateVector(s.StageOutputStateIndices);
            auto uniformBufferStateIndicesOffset = fbb.CreateVector(s.UniformBufferStateIndices);
            auto pushConstantBufferStateIndicesOffset = fbb.CreateVector(s.PushConstantBufferStateIndices);
            auto sampleImageStateIndicesOffset = fbb.CreateVector(s.SampledImageStateIndices);
            auto subpassInputStateIndicesOffset = fbb.CreateVector(s.SubpassInputStateIndices);
            auto separateImageStateIndicesOffset = fbb.CreateVector(s.SeparateImageStateIndices);
            auto separateSamplerStateIndicesOffset = fbb.CreateVector(s.SeparateSamplerStateIndices);
            auto storageImageStateIndicesOffset = fbb.CreateVector(s.StorageImageStateIndices);
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
                                                                        stageInputStateIndicesOffset,
                                                                        stageOutputStateIndicesOffset,
                                                                        uniformBufferStateIndicesOffset,
                                                                        pushConstantBufferStateIndicesOffset,
                                                                        sampleImageStateIndicesOffset,
                                                                        subpassInputStateIndicesOffset,
                                                                        separateImageStateIndicesOffset,
                                                                        separateSamplerStateIndicesOffset,
                                                                        storageImageStateIndicesOffset,
                                                                        storageBufferStateIndicesOffset));
        }

        reflectedShadersOffset = fbb.CreateVector(reflectedShaderOffsets);

        std::vector<cso::CompiledShader> compiledShaderOffsets = {};
        for (auto& compiledShader : uniqueCompiledShaders) {
            compiledShaderOffsets.push_back(cso::CompiledShader(compiledShader.BufferIndex,
                                                                compiledShader.ReflectedIndex,
                                                                compiledShader.PreprocessedIndex,
                                                                compiledShader.AssemblyIndex,
                                                                compiledShader.VulkanIndex,
                                                                compiledShader.ES2Index,
                                                                compiledShader.ES3Index,
                                                                compiledShader.iOSIndex,
                                                                compiledShader.macOSIndex,
                                                                cso::IR_SPIRV));
        }

        compiledShadersOffset = fbb.CreateVectorOfStructs(compiledShaderOffsets.data(), compiledShaderOffsets.size());

        std::vector<flatbuffers::Offset<cso::CompiledShaderInfo>> compiledShaderInfoOffsets = {};
        for (auto& compiledShaderInfo : uniqueCompiledShaderInfos) {
            flatbuffers::Offset<flatbuffers::Vector<uint32_t>> includedFilesOffset = fbb.CreateVector(
                compiledShaderInfo.IncludedFileIndices.data(), compiledShaderInfo.IncludedFileIndices.size());
            flatbuffers::Offset<flatbuffers::Vector<uint32_t>> definitionsOffset = fbb.CreateVector(
                compiledShaderInfo.DefinitionIndices.data(), compiledShaderInfo.DefinitionIndices.size());
            compiledShaderInfoOffsets.push_back(
                cso::CreateCompiledShaderInfo(fbb,
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

    void Pack(const std::vector<std::unique_ptr<CompiledShaderVariant>>& variants) {
        for (auto& csoPtr : variants) {
            auto& cso = *csoPtr;
            HashedCompiledShader compiledShader = {};
            compiledShader.BufferIndex = GetBufferIndex(cso.Buffer);
            compiledShader.PreprocessedIndex = GetStringIndex(cso.Preprocessed);
            compiledShader.AssemblyIndex = GetStringIndex(cso.Assembly);
            compiledShader.VulkanIndex = GetStringIndex(cso.Vulkan);
            compiledShader.iOSIndex = GetStringIndex(cso.iOS);
            compiledShader.macOSIndex = GetStringIndex(cso.macOS);
            compiledShader.ES2Index = GetStringIndex(cso.ES2);
            compiledShader.ES3Index = GetStringIndex(cso.ES3);
            compiledShader.ReflectedIndex = GetReflectedShaderIndex(GetHashedReflectionShader(cso.Reflected));
            compiledShader.Hash = uniqueBuffers[compiledShader.BufferIndex].Hash;

            const uint32_t compiledShaderIndex = GetCompiledShaderIndex(compiledShader);

            apemode::CityHasher64 city64 = {};
            HashedCompiledShaderInfo compiledShaderInfo = {};

            compiledShaderInfo.CompiledShaderIndex = compiledShaderIndex;
            compiledShaderInfo.AssetIndex = GetStringIndex(cso.Asset);
            compiledShaderInfo.DefinitionsIndex = GetStringIndex(cso.Definitions);
            compiledShaderInfo.ShaderType = cso.Type;

            city64.CombineWith(compiledShaderInfo.ShaderType);
            city64.CombineWith(uniqueCompiledShaders[compiledShaderInfo.CompiledShaderIndex].Hash);
            city64.CombineWith(GetStringHash(compiledShaderInfo.AssetIndex));
            city64.CombineWith(GetStringHash(compiledShaderInfo.DefinitionsIndex));

            for (auto& includedFile : cso.IncludedFiles) {
                const uint32_t stringIndex = GetStringIndex(includedFile);
                city64.CombineWith(GetStringHash(stringIndex));
                compiledShaderInfo.IncludedFileIndices.push_back(stringIndex);
            }

            for (auto& definitionPair : cso.DefinitionMap) {
                const uint32_t stringIndex0 = GetStringIndex(definitionPair.first);
                const uint32_t stringIndex1 = GetStringIndex(definitionPair.second);
                city64.CombineWith(GetStringHash(stringIndex0));
                city64.CombineWith(GetStringHash(stringIndex1));
                compiledShaderInfo.DefinitionIndices.push_back(stringIndex0);
                compiledShaderInfo.DefinitionIndices.push_back(stringIndex1);
            }

            compiledShaderInfo.Hash = city64;
            GetCompiledShaderInfoIndex(compiledShaderInfo);
        }
    }

    // clang-format off
    template <typename T>
    static uint32_t TAddIfMissingAndGetIndexByHash(std::vector<T>& existingReflectedItems, const T& reflectedItem) {
        static_assert(std::is_base_of<Hashed, T>::value, "Caught T without a hash field.");
        const auto it = std::find_if(existingReflectedItems.cbegin(), existingReflectedItems.cend(), [reflectedItem](const T& existingItem) { return existingItem.Hash == reflectedItem.Hash; });
        if (it != existingReflectedItems.end()) { return std::distance(existingReflectedItems.cbegin(), it); }
        const uint32_t index = existingReflectedItems.size();
        existingReflectedItems.push_back(reflectedItem);
        return index;
    }
    
    uint64_t GetStringHash(const uint32_t index) { return uniqueStrings[index].Hash; }
    uint64_t GetTypeHash(const uint32_t index) { return uniqueReflectedTypes[index].Hash; }
    // clang-format on

    uint32_t GetCompiledShaderInfoIndex(const HashedCompiledShaderInfo& reflected) {
        return TAddIfMissingAndGetIndexByHash(uniqueCompiledShaderInfos, reflected);
    }
    HashedReflectedResourceState GetHashedReflectedResourceState(
        const apemode::shp::ReflectedResource& reflectedResource) {
        HashedReflectedResourceState reflectedState = {};

        reflectedState.bIsActive = reflectedResource.bIsActive;
        reflectedState.ActiveRanges.reserve(reflectedResource.ActiveRanges.size());

        // clang-format off
        std::transform(reflectedResource.ActiveRanges.begin(),
                       reflectedResource.ActiveRanges.end(),
                       std::back_inserter(reflectedState.ActiveRanges),
                       [](apemode::shp::ReflectedMemoryRange r) { return std::make_pair(r.offset, r.size); });
        // clang-format on

        apemode::CityHasher64 city64 = {};
        city64.CombineWith(reflectedState.bIsActive);
        if (!reflectedResource.ActiveRanges.empty()) {
            city64.CombineWithArray(reflectedResource.ActiveRanges.data(), reflectedResource.ActiveRanges.size());
        }

        reflectedState.Hash = city64;
        return reflectedState;
    }
    uint32_t GetReflectedResourceStateIndex(const HashedReflectedResourceState& reflected) {
        return TAddIfMissingAndGetIndexByHash(uniqueReflectedResourceStates, reflected);
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

        apemode::CityHasher64 city64 = {};
        city64.CombineWith(GetStringHash(hashedReflectedType.NameIndex));
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
            city64.CombineWith(GetStringHash(members.NameIndex));
            city64.CombineWith(GetTypeHash(members.TypeIndex));
            city64.CombineWith(members.ByteOffset);
            city64.CombineWith(members.EffectiveByteSize);
            city64.CombineWith(members.OccupiedByteSize);
        }

        hashedReflectedType.Hash = city64;
        return hashedReflectedType;
    }
    uint32_t GetReflectedTypeIndex(const HashedReflectedType& reflected) {
        return TAddIfMissingAndGetIndexByHash(uniqueReflectedTypes, reflected);
    }
    HashedReflectedResource GetHashedReflectedResource(const apemode::shp::ReflectedResource& reflectedResource) {
        HashedReflectedType hashedType = GetHashedReflectedType(reflectedResource.Type);

        HashedReflectedResource hashedReflectedResource = {};
        hashedReflectedResource.NameIndex = GetStringIndex(reflectedResource.Name);
        hashedReflectedResource.TypeIndex = GetReflectedTypeIndex(hashedType);
        hashedReflectedResource.DescriptorSet = reflectedResource.DecorationDescriptorSet;
        hashedReflectedResource.DescriptorBinding = reflectedResource.DecorationBinding;
        hashedReflectedResource.Locaton = reflectedResource.DecorationLocation;

        apemode::CityHasher64 city64 = {};
        city64.CombineWith(GetStringHash(hashedReflectedResource.NameIndex));
        city64.CombineWith(GetTypeHash(hashedReflectedResource.TypeIndex));
        city64.CombineWith(hashedReflectedResource.DescriptorSet);
        city64.CombineWith(hashedReflectedResource.DescriptorBinding);
        city64.CombineWith(hashedReflectedResource.Locaton);

        hashedReflectedResource.Hash = city64;
        return hashedReflectedResource;
    }
    uint32_t GetReflectedResourceIndex(const HashedReflectedResource& reflected) {
        return TAddIfMissingAndGetIndexByHash(uniqueReflectedResources, reflected);
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

        apemode::CityHasher64 city64 = {};
        city64.CombineWith(GetStringHash(hashedReflectedConstant.NameIndex));
        city64.CombineWith(GetStringHash(hashedReflectedConstant.MacroIndex));
        city64.CombineWith(GetTypeHash(hashedReflectedConstant.TypeIndex));
        city64.CombineWith(hashedReflectedConstant.ConstantId);
        city64.CombineWith(hashedReflectedConstant.DefaultScalarU64);
        city64.CombineWith(hashedReflectedConstant.bIsSpecialization);
        city64.CombineWith(hashedReflectedConstant.bIsUsedAsArrayLength);
        city64.CombineWith(hashedReflectedConstant.bIsUsedAsLUT);

        hashedReflectedConstant.Hash = city64;
        return hashedReflectedConstant;
    }
    uint32_t GetReflectedConstantIndex(const HashedReflectedConstant& reflected) {
        return TAddIfMissingAndGetIndexByHash(uniqueReflectedConstants, reflected);
    }

    void AddReflectedResources(const std::vector<apemode::shp::ReflectedResource>& reflectedResources,
                               std::vector<uint32_t>& resourceIndices,
                               std::vector<uint32_t>& resourceStateIndices,
                               apemode::CityHasher64& city64) {
        for (auto& reflectedResource : reflectedResources) {
            HashedReflectedResource hashedResource = GetHashedReflectedResource(reflectedResource);
            resourceIndices.push_back(GetReflectedResourceIndex(hashedResource));
            city64.CombineWith(hashedResource.Hash);

            HashedReflectedResourceState hashedState = GetHashedReflectedResourceState(reflectedResource);
            resourceStateIndices.push_back(GetReflectedResourceStateIndex(hashedState));
            city64.CombineWith(hashedState.Hash);
        }
    }

    HashedReflectedShader GetHashedReflectionShader(const apemode::shp::ReflectedShader& reflectedShader) {
        HashedReflectedShader hashedReflectedShader = {};
        apemode::CityHasher64 city64 = {};

        hashedReflectedShader.NameIndex = GetStringIndex(reflectedShader.Name);
        city64.CombineWith(GetStringHash(hashedReflectedShader.NameIndex));

        for (auto& reflectedConstant : reflectedShader.Constants) {
            HashedReflectedConstant hashedConstant = GetHashedReflectedConstant(reflectedConstant);
            hashedReflectedShader.ConstantIndices.push_back(GetReflectedConstantIndex(hashedConstant));
            city64.CombineWith(hashedConstant.Hash);
        }

        AddReflectedResources(reflectedShader.StageInputs,
                              hashedReflectedShader.StageInputIndices,
                              hashedReflectedShader.StageInputStateIndices,
                              city64);
        AddReflectedResources(reflectedShader.StageOutputs,
                              hashedReflectedShader.StageOutputIndices,
                              hashedReflectedShader.StageOutputStateIndices,
                              city64);
        AddReflectedResources(reflectedShader.UniformBuffers,
                              hashedReflectedShader.UniformBufferIndices,
                              hashedReflectedShader.UniformBufferStateIndices,
                              city64);
        AddReflectedResources(reflectedShader.PushConstantBuffers,
                              hashedReflectedShader.PushConstantBufferIndices,
                              hashedReflectedShader.PushConstantBufferStateIndices,
                              city64);

        AddReflectedResources(reflectedShader.SampledImages,
                              hashedReflectedShader.SampledImageIndices,
                              hashedReflectedShader.SampledImageStateIndices,
                              city64);

        AddReflectedResources(reflectedShader.SubpassInputs,
                              hashedReflectedShader.SubpassInputIndices,
                              hashedReflectedShader.SubpassInputStateIndices,
                              city64);

        AddReflectedResources(reflectedShader.SeparateImages,
                              hashedReflectedShader.SeparateImageIndices,
                              hashedReflectedShader.SeparateImageStateIndices,
                              city64);

        AddReflectedResources(reflectedShader.SeparateSamplers,
                              hashedReflectedShader.SeparateSamplerIndices,
                              hashedReflectedShader.SeparateSamplerStateIndices,
                              city64);

        AddReflectedResources(reflectedShader.StorageImages,
                              hashedReflectedShader.StorageImageIndices,
                              hashedReflectedShader.StorageImageStateIndices,
                              city64);

        AddReflectedResources(reflectedShader.StorageBuffers,
                              hashedReflectedShader.StorageBufferIndices,
                              hashedReflectedShader.StorageBufferStateIndices,
                              city64);

        hashedReflectedShader.Hash = city64;
        return hashedReflectedShader;
    }
    uint32_t GetReflectedShaderIndex(const HashedReflectedShader& reflected) {
        return TAddIfMissingAndGetIndexByHash(uniqueReflectedShaders, reflected);
    }
    uint32_t GetCompiledShaderIndex(const HashedCompiledShader& compiledShader) {
        return TAddIfMissingAndGetIndexByHash(uniqueCompiledShaders, compiledShader);
    }
    // clang-format off
    uint32_t GetStringIndex(const std::string& string) {
        uint64_t hash = apemode::CityHash64(string.data(), string.size());
        auto it = std::find_if(uniqueStrings.begin(), uniqueStrings.end(), [hash](const UniqueString& existing) { return existing.Hash == hash; });
        if (it != uniqueStrings.end()) { return std::distance(uniqueStrings.begin(), it); }
        const uint32_t index = uniqueStrings.size();
        uniqueStrings.push_back({{hash}, string});
        return index;
    }
    uint32_t GetBufferIndex(const std::vector<uint8_t>& buffer) {
        uint64_t hash = apemode::CityHash64((const char*)buffer.data(), buffer.size());
        auto it = std::find_if(uniqueBuffers.begin(), uniqueBuffers.end(), [hash](const UniqueBuffer& existing) { return existing.Hash == hash; });
        if (it != uniqueBuffers.end()) { return std::distance(uniqueBuffers.begin(), it); }
        const uint32_t index = uniqueBuffers.size();
        uniqueBuffers.push_back({{hash}, buffer});
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
    std::vector<std::string> Paths;

    bool ReadShaderTxtFile(const std::string& InFilePath,
                           std::string& OutFileFullPath,
                           std::string& OutFileContent,
                           bool bRelative) {
        if (bRelative) {
            auto dir = std::filesystem::path(Paths.back());
            auto file = dir / InFilePath;

            OutFileFullPath = std::filesystem::absolute(file).string();
            if (std::filesystem::exists(file)) {
                // TODO: ReadTxtFile should return a string.
                OutFileContent = ReadTextFile(file.string());
                return true;
            }
        }

        for (auto path : Paths) {
            auto file = std::filesystem::path(path) / OutFileFullPath;
            if (std::filesystem::exists(file)) {
                OutFileContent = ReadTextFile(file.string());
                return true;
            }
        }

        assert(false && "Caught file not found.");
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
            apemode::LogError("ShaderCompiler: {}/{}: {}", feedbackStage, feedbackCompilationError, FullFilePath);
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
    for (auto c : s) { containsWhitespace |= isspace(c) != 0; }

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
    } else if (shaderType == "rahit") {
        return apemode::shp::IShaderCompiler::ShaderType::AnyHit;
    } else if (shaderType == "rchit") {
        return apemode::shp::IShaderCompiler::ShaderType::ClosestHit;
    } else if (shaderType == "rmiss") {
        return apemode::shp::IShaderCompiler::ShaderType::Miss;
    } else if (shaderType == "rcall") {
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

void DumpCompiledShaderTarget(const apemode::shp::ICompiledShader* compiledShader, std::string outputPath, apemode::shp::CompiledShaderTarget target) {
    if (compiledShader->HasSourceFor(target)) {
        auto sourceForTarget = compiledShader->GetSourceFor(target);
        flatbuffers::SaveFile(outputPath.c_str(), sourceForTarget.data(), sourceForTarget.size(), false);
    } else {
        auto errorForTarget = compiledShader->GetErrorFor(target);
        flatbuffers::SaveFile((outputPath+"-err.txt").c_str(), errorForTarget.data(), errorForTarget.size(), false);
    }
}

void DumpCompiledShader(const apemode::shp::ICompiledShader* compiledShader, std::string outputFolder, std::string srcFile, std::string macrosString) {
    ReplaceAll(macrosString, ".", "-");
    ReplaceAll(macrosString, ";", "+");

    std::string dstFile = srcFile;
    const size_t fileStartPos = dstFile.find_last_of("/\\");
    if (fileStartPos != dstFile.npos) { dstFile = dstFile.substr(fileStartPos + 1); }

    // clang-format off
    std::string dstFilePath = outputFolder + "/" + dstFile + (macrosString.empty() ? "" : "-defs-") + macrosString + ".spv";
    // clang-format on

    ReplaceAll(dstFilePath, "//", "/");
    ReplaceAll(dstFilePath, "\\/", "\\");
    ReplaceAll(dstFilePath, "\\\\", "\\");

    const std::string cachedPreprocessed = dstFilePath + "-preprocessed.txt";
    const std::string cachedAssembly = dstFilePath + "-assembly.txt";
    const std::string cachedVulkan = dstFilePath + "-glsl-vulkan.txt";
    const std::string cachedES2 = dstFilePath + "-glsl-es2.txt";
    const std::string cachedES3 = dstFilePath + "-glsl-es3.txt";
    const std::string cachediOS = dstFilePath + "-msl-ios.txt";
    const std::string cachedmacOS = dstFilePath + "-msl-macos.txt";

    // clang-format off
    flatbuffers::SaveFile(dstFilePath.c_str(), (const char*)compiledShader->GetBytePtr(), compiledShader->GetByteCount(), true);
    // clang-format on
    
    DumpCompiledShaderTarget(compiledShader, cachedPreprocessed, apemode::shp::CompiledShaderTarget::Preprocessed);
    DumpCompiledShaderTarget(compiledShader, cachedAssembly, apemode::shp::CompiledShaderTarget::Assembly);
    DumpCompiledShaderTarget(compiledShader, cachedVulkan, apemode::shp::CompiledShaderTarget::Vulkan);
    DumpCompiledShaderTarget(compiledShader, cachedES2, apemode::shp::CompiledShaderTarget::ES2);
    DumpCompiledShaderTarget(compiledShader, cachedES3, apemode::shp::CompiledShaderTarget::ES3);
    DumpCompiledShaderTarget(compiledShader, cachediOS, apemode::shp::CompiledShaderTarget::iOS);
    DumpCompiledShaderTarget(compiledShader, cachedmacOS, apemode::shp::CompiledShaderTarget::macOS);
}

std::unique_ptr<CompiledShaderVariant> CompileShaderVariant(const apemode::shp::IShaderCompiler& shaderCompiler,
                                                            const std::map<std::string, std::string>& macroDefinitions,
                                                            const std::string& shaderType,
                                                            const std::string& srcFile,
                                                            const std::string& outputFolder) {
    std::string macrosString = GetMacrosString(macroDefinitions);
    apemode::LogInfo("Variant: asset=\"{}\", definitions=\"{}\"", srcFile, macrosString);

    auto csoPtr = std::make_unique<CompiledShaderVariant>();
    auto& cso = *csoPtr;
    // CompiledShaderVariant cso = {};
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
                                                     apemode::shp::IShaderCompiler::ShaderOptimizationType::Performance,
                                                     &includedFileSet)) {
        cso.Buffer.assign(compiledShader->GetBytePtr(),
                            compiledShader->GetBytePtr() + compiledShader->GetByteCount());

        cso.Preprocessed = compiledShader->GetSourceFor(apemode::shp::CompiledShaderTarget::Preprocessed);
        cso.Assembly = compiledShader->GetSourceFor(apemode::shp::CompiledShaderTarget::Assembly);
        cso.Vulkan = compiledShader->GetSourceFor(apemode::shp::CompiledShaderTarget::Vulkan);
        cso.iOS = compiledShader->GetSourceFor(apemode::shp::CompiledShaderTarget::iOS);
        cso.macOS = compiledShader->GetSourceFor(apemode::shp::CompiledShaderTarget::macOS);
        cso.ES2 = compiledShader->GetSourceFor(apemode::shp::CompiledShaderTarget::ES2);
        cso.ES3 = compiledShader->GetSourceFor(apemode::shp::CompiledShaderTarget::ES3);
        cso.Asset = srcFile;
        cso.IncludedFiles = includedFileSet.IncludedFiles;
        cso.DefinitionMap = macroDefinitions;
        cso.Definitions = macrosString;
        cso.Reflected = std::move(compiledShader->GetReflection());
        cso.Type = cso::Shader(eShaderType);
        
        DumpCompiledShader(compiledShader.get(), outputFolder, srcFile, macrosString);
        return csoPtr;
    }

    return {};
}

void CompileShaderVariantsRecursively(std::vector<std::unique_ptr<CompiledShaderVariant>>& csos,
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
            csos.emplace_back(std::move(cso));
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

void CompileShaderVariantsRecursively(std::vector<std::unique_ptr<CompiledShaderVariant>>& csos,
                                      const apemode::shp::IShaderCompiler& shaderCompiler,
                                      const ShaderCompilerMacroGroupCollection& macroGroups,
                                      const std::string& shaderType,
                                      const std::string& srcFile,
                                      const std::string& outputFolder) {
    std::vector<size_t> macroIndices(macroGroups.GetCount());
    CompileShaderVariantsRecursively(
        csos, shaderCompiler, macroGroups, 0, shaderType, srcFile, outputFolder, macroIndices);
}

std::vector<std::unique_ptr<CompiledShaderVariant>> CompileShaderType(
    //    const apemode::platform::shared::AssetManager& assetManager,
    apemode::shp::IShaderCompiler& shaderCompiler,
    const json& commandJson,
    const std::string& outputFolder,
    const std::string& shaderType) {
    std::vector<std::unique_ptr<CompiledShaderVariant>> csos;

    assert(commandJson["srcFile"].is_string());
    std::string srcFile = commandJson["srcFile"].get<std::string>();

    auto definitionGroupsJsonIt = commandJson.find("definitionGroups");
    if (definitionGroupsJsonIt != commandJson.end() && definitionGroupsJsonIt->is_array()) {
        ShaderCompilerMacroGroupCollection macroGroups;

        const json& definitionGroupsJson = *definitionGroupsJsonIt;
        for (auto& definitionGroupJson : definitionGroupsJson) {
            macroGroups.MacroGroups.push_back(GetMacroGroup(definitionGroupJson));
        }

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

        if (auto cso = CompileShaderVariant(shaderCompiler, macroDefinitions, shaderType, srcFile, outputFolder)) {
            csos.emplace_back(std::move(cso));
        }
    }

    return csos;
}

std::vector<std::unique_ptr<CompiledShaderVariant>> CompileShader(
    //    const apemode::platform::shared::AssetManager& assetManager,
    apemode::shp::IShaderCompiler& shaderCompiler,
    const json& commandJson,
    const std::string& outputFolder) {
    if (commandJson["shaderType"].is_string()) {
        const std::string shaderType = commandJson["shaderType"].get<std::string>();
        return CompileShaderType(shaderCompiler, commandJson, outputFolder, shaderType);

    } else if (commandJson["shaderType"].is_array()) {
        std::vector<std::unique_ptr<CompiledShaderVariant>> variants;

        for (auto& shaderTypeObj : commandJson["shaderType"]) {
            assert(shaderTypeObj.is_string());
            std::string shaderType = shaderTypeObj.get<std::string>();
            auto newVariants = CompileShaderType(shaderCompiler, commandJson, outputFolder, shaderType);
            variants.insert(variants.end(),
                            std::make_move_iterator(newVariants.begin()),
                            std::make_move_iterator(newVariants.end()));
        }

        return variants;
    }

    apemode::LogError("Invalid shader type, the command \"{}\" skipped.", commandJson.dump().c_str());
    return {};
}
} // namespace

template <int TPrecision = 100>
float RoundOff(float n) {
    const float i = n * static_cast<float>(TPrecision) + 0.5f;
    return ((float)(int)i) / static_cast<float>(TPrecision);
}

// clang-format off
std::string ToPrettySizeString(size_t size) {
    constexpr std::array<std::string_view, 4> kSizeStrings = {"B", "KB", "MB", "GB"};
    size_t div = 0;
    size_t rem = 0;
    while (size >= 1024 && div < kSizeStrings.size()) { rem = size % 1024 ; div++; size /= 1024; }
    const float size_d = static_cast<float>(size) +  static_cast<float>( rem ) / 1024.0f;
    
    std::array<char, 16> buff = {};
    sprintf(buff.data(), "%g %s", RoundOff(size_d), kSizeStrings[div].data());

    // std::string string = std::to_string(RoundOff(size_d));
    // string += kSizeStrings[div];

    return buff.data();
}
// clang-format on

std::string ToHeaderFile(std::string name, const char* pBuffer, size_t bufferLength, uint64_t hash) {
    std::string headerContents = {};
    headerContents.reserve(bufferLength << 1);

    headerContents += "//\n// Generated by PrecompiledShaderPipeline\n//\n\n";
    headerContents += "#include <cstdint>\n";
    headerContents += "#include <string_view>\n\n";
    headerContents += "namespace apemode::cso::embedded {\n";
    headerContents += "#ifndef " + name + "_declarations\n";
    headerContents += "#define " + name + "_declarations\n";
    headerContents += "extern uint64_t get_" + name + "_buffer_hash();\n";
    headerContents += "extern std::string_view get_" + name + "_buffer_view();\n";
    headerContents += "#endif //" + name + "_declarations\n\n";
    headerContents += "#ifdef " + name + "_paste_implementation\n";
    headerContents += "const size_t " + name + "_byte_size=" + std::to_string(bufferLength) + ";\n";

    std::array<char, 64> buff = {0};
    if (bufferLength % 8 == 0) {
        size_t count = bufferLength / 8;
        headerContents += "const uint64_t " + name + "_array[" + std::to_string(count) + "]={";

        auto ints = reinterpret_cast<const uint64_t*>(pBuffer);
        for (size_t i = 0; i < count; ++i) {
            sprintf(buff.data(), "0x%llx,", ints[i]);
            headerContents += buff.data();
        }
    } else if (bufferLength % 4 == 0) {
        size_t count = bufferLength / 4;
        headerContents += "const uint32_t " + name + "_array[" + std::to_string(count) + "]={";

        auto ints = reinterpret_cast<const uint32_t*>(pBuffer);
        for (size_t i = 0; i < count; ++i) {
            sprintf(buff.data(), "0x%x,", ints[i]);
            headerContents += buff.data();
        }
    } else if (bufferLength % 2 == 0) {
        size_t count = bufferLength / 2;
        headerContents += "const uint16_t " + name + "_array[" + std::to_string(count) + "]={";

        auto ints = reinterpret_cast<const uint16_t*>(pBuffer);
        for (size_t i = 0; i < count; ++i) {
            sprintf(buff.data(), "0x%x,", static_cast<uint32_t>(ints[i]) & 0xffff);
            headerContents += buff.data();
        }
    } else {
        headerContents += "const uint8_t " + name + "_array[" + std::to_string(bufferLength) + "]={";
        for (size_t i = 0; i < bufferLength; ++i) {
            sprintf(buff.data(), "0x%x,", static_cast<uint32_t>(pBuffer[i]) & 0xff);
            headerContents += buff.data();
        }
    }

    // clang-format off
    headerContents.back() = '}';
    headerContents += ";\nstatic_assert(sizeof(" + name + "_array) == " + name + "_byte_size, \"Caught size mismatch.\");\n\n";

    headerContents += "uint64_t get_" + name + "_buffer_hash(){\n";
    headerContents += "return " + std::to_string(hash) + ";\n";
    headerContents += "}\n";
    headerContents += "std::string_view get_" + name + "_buffer_view(){\n";
    headerContents += "return std::string_view(reinterpret_cast<const char*>(" + name + "_array), " + name + "_byte_size);\n";
    headerContents += "}\n\n#endif //" + name + "_paste_implementation\n\n";
    headerContents += "}\n"; // namespace apemode::cso::embedded
    // clang-format on

    return headerContents;
}

int BuildLibrary(int argc, char** argv) {
    apemode::AppState::OnMain(argc, (const char**)argv);
    apemode::AppStateExitGuard eg{};

    if (!apemode::AppState::Get() || !apemode::AppState::Get()->GetArgs()) { return -1; }

    auto& options = *apemode::AppState::Get()->GetArgs();

    std::string mode = options["mode"].as<std::string>();
    if (mode != "build-collection") { return 1; }

    std::string inputFile = options["input-file"].as<std::string>();
    if (!std::filesystem::exists(inputFile)) { return 1; }

    std::string outputFile = options["output-file"].as<std::string>();
    if (outputFile.empty()) { return 1; }

    std::string outputFolder = outputFile + ".d";
    if (!std::filesystem::exists(outputFolder)) { std::filesystem::create_directory(outputFolder); }
    if (!std::filesystem::exists(outputFolder)) { return 1; }

    if (outputFile.empty()) {
        apemode::LogError("Output CSO file is empty.");
        return 1;
    }

    apemode::LogInfo("CSO JSON file: {}", inputFile);

    auto csoJsonContents = ReadTextFile(inputFile);
    if (csoJsonContents.empty()) {
        apemode::LogError("CSO file is empty.");
        return 1;
    }

    const json csoJson = json::parse(csoJsonContents.c_str());
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

    if (options.count("add-path")) {
        shaderCompilerFileReader.Paths = options["add-path"].as<std::vector<std::string>>();
    }

    shaderCompilerFileReader.Paths.push_back(std::filesystem::path(inputFile).parent_path().string());
    shaderCompiler->SetShaderFileReader(&shaderCompilerFileReader);
    shaderCompiler->SetShaderFeedbackWriter(&shaderFeedbackWriter);

    std::vector<std::unique_ptr<CompiledShaderVariant>> compiledShaders;

    // clang-format off
    const json& commandsJson = csoJson["commands"];
    for (const auto& commandJson : commandsJson) {
        std::vector<std::unique_ptr<CompiledShaderVariant>> variants = CompileShader(*shaderCompiler, commandJson, outputFolder);
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

    apemode::LogInfo("= {} bytes ~ {}", builtBuffeLen, ToPrettySizeString(builtBuffeLen));
    apemode::LogInfo("CSO file: {}", outputFile);
    if (!flatbuffers::SaveFile(outputFile.c_str(), builtBuffePtr, builtBuffeLen, true)) {
        apemode::LogError("Failed to write CSO ({} bytes) to file: '{}'", builtBuffeLen, outputFile);
        return 1;
    }

    constexpr bool exportHeader = true;
    if constexpr (exportHeader) {
        std::string name = std::filesystem::path(outputFile).filename().string();
        ReplaceAll(name, ".", "_");
        ReplaceAll(name, " ", "_");
        ReplaceAll(name, "/", "_");
        ReplaceAll(name, "\\", "_");
        std::for_each(name.begin(), name.end(), [](char& c) { c = std::tolower(c); });

        auto hash = apemode::CityHash64(builtBuffePtr, builtBuffeLen);
        auto hashString = std::to_string(hash);
        auto header = ToHeaderFile(name, builtBuffePtr, builtBuffeLen, hash);
        flatbuffers::SaveFile((outputFile + ".h").c_str(), header.data(), header.size(), false);
        flatbuffers::SaveFile((outputFile + ".hash.bin").c_str(), (const char*)&hash, sizeof(hash), true);
        flatbuffers::SaveFile((outputFile + ".hash.txt").c_str(), hashString.c_str(), hashString.size(), false);
    }

    apemode::LogInfo("Done.");
    return 0;
}
