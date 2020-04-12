#pragma once

#include <cso_generated.h>
#include <flatbuffers/flatbuffers.h>

#include <cassert>
#include <vector>

namespace apemode {} // namespace apemode

namespace cso::utils {
namespace {
constexpr std::string_view EMPTY_STRING = {nullptr, 0};

std::string_view GetStringView(flatbuffers::string_view sv) {
    if constexpr (std::is_same_v<flatbuffers::string_view, std::string_view>) { return sv; }
    return std::string_view(sv.data(), sv.size());
}

std::string_view GetStringViewAtIndex(const cso::CompiledShaderCollection* pCollection, size_t index) {
    if (!pCollection) { return EMPTY_STRING; }
    if (!pCollection->strings()) { return EMPTY_STRING; }
    if (index >= pCollection->strings()->size()) { return EMPTY_STRING; }
    const cso::UniqueString* pUniqueString = pCollection->strings()->Get(index);
    return GetStringView(pUniqueString->contents()->string_view());
}
} // namespace

template <typename T>
struct ArrayView {
    T* pElements = nullptr;
    size_t Count = 0;

    T* data() const { return pElements; }
    size_t size() const { return Count; }
    T* begin() const { return pElements; }
    T* end() const { return pElements + Count; }
};

struct Definition {
    std::string_view DefinitionName = {};
    std::string_view DefinitionValue = {};
    int64_t ToInt() const { return std::atoll(DefinitionValue.data()); }
    double ToDouble() const { return std::atof(DefinitionValue.data()); }
};

struct TypeDescription {
    const cso::CompiledShaderCollection* pCollection = nullptr;
    const cso::ReflectedType* pType = nullptr;

    std::string_view Name() const {
        if (!pType) { return {}; }
        return GetStringViewAtIndex(pCollection, pType->name_index());
    }
    cso::ReflectedPrimitiveType PrimitiveType() const {
        if (!pType) { return {}; }
        return pType->element_primitive_type();
    }
    size_t VectorLength() const {
        if (!pType) { return {}; }
        return pType->element_vector_length();
    }
    size_t ColumnCount() const {
        if (!pType) { return {}; }
        return pType->element_column_count();
    }
    size_t ArrayLength() const {
        if (!pType) { return 0; }
        return pType->array_length_with_bits() & cso::ArrayLength_ValueBitMask;
    }
    bool IsArray() const {
        if (!pType) { return false; }
        return ArrayLength() > 1;
    }
    bool IsStaticArray() const {
        if (!pType) { return false; }
        return 0 != (pType->array_length_with_bits() & cso::ArrayLength_IsStaticBitMask);
    }
    bool IsSeparateImage() const {
        if (!pType) { return false; }
        return pType->element_primitive_type() == cso::ReflectedPrimitiveType_Image;
    }
    bool IsSeparateSampler() const {
        if (!pType) { return false; }
        return pType->element_primitive_type() == cso::ReflectedPrimitiveType_Sampler;
    }
    bool IsCombinedSampledImage() const {
        if (!pType) { return false; }
        return pType->element_primitive_type() == cso::ReflectedPrimitiveType_SampledImage;
    }
    bool IsStruct() const {
        if (!pType) { return false; }
        return pType->element_primitive_type() == cso::ReflectedPrimitiveType_Struct;
    }
    size_t MemberCount() const {
        if (!pType) { return 0; }
        if (!pType->member_types()) { return 0; }
        return pType->member_types()->size();
    }
    std::string_view MemberName(size_t index) const {
        if (!pType) { return EMPTY_STRING; }
        if (!pType->member_types()) { return EMPTY_STRING; }
        auto pMember = pType->member_types()->Get(index);
        return GetStringViewAtIndex(pCollection, pMember->name_index());
    }
    size_t MemberEffectiveByteSize(size_t index) const {
        if (!pType) { return 0; }
        if (!pType->member_types()) { return 0; }
        auto pMember = pType->member_types()->Get(index);
        return pMember->effective_byte_size();
    }
    size_t MemberOccupiedByteSize(size_t index) const {
        if (!pType) { return 0; }
        if (!pType->member_types()) { return 0; }
        auto pMember = pType->member_types()->Get(index);
        return pMember->occupied_byte_size();
    }
    size_t MemberByteOffset(size_t index) const {
        if (!pType) { return size_t(-1); }
        if (!pType->member_types()) { return size_t(-1); }
        auto pMember = pType->member_types()->Get(index);
        return pMember->byte_offset();
    }
    TypeDescription MemberType(size_t index) const {
        if (!pCollection) { return {}; }
        if (!pType) { return {}; }
        if (!pType->member_types()) { return {}; }
        auto pMember = pType->member_types()->Get(index);
        auto pType = pCollection->reflected_types()->Get(pMember->type_index());
        return {pCollection, pType};
    }

    bool IsIntType() const { return pType && pType->element_primitive_type() == cso::ReflectedPrimitiveType_Int; }
    bool IsFloatType() const { return pType && pType->element_primitive_type() == cso::ReflectedPrimitiveType_Float; }

    bool IsInt() const {
        return IsIntType() && pType->element_vector_length() == 1 && pType->element_column_count() == 1;
    }
    bool IsFloat() const {
        return IsFloatType() && pType->element_vector_length() == 1 && pType->element_column_count() == 1;
    }
    bool IsFloatVector2() const {
        return IsFloatType() && pType->element_vector_length() == 2 && pType->element_column_count() == 1;
    }
    bool IsFloatVector3() const {
        return IsFloatType() && pType->element_vector_length() == 3 && pType->element_column_count() == 1;
    }
    bool IsFloatVector4() const {
        return IsFloatType() && pType->element_vector_length() == 4 && pType->element_column_count() == 1;
    }
    bool IsFloatMatrix2x2() const {
        return IsFloatType() && pType->element_vector_length() == 2 && pType->element_column_count() == 2;
    }
    bool IsFloatMatrix3x3() const {
        return IsFloatType() && pType->element_vector_length() == 3 && pType->element_column_count() == 3;
    }
    bool IsFloatMatrix4x4() const {
        return IsFloatType() && pType->element_vector_length() == 4 && pType->element_column_count() == 4;
    }
    bool IsFloatMatrix(size_t rows, size_t columns) const {
        return IsFloatType() && pType->element_vector_length() == rows && pType->element_column_count() == columns;
    }
};

struct PrecompiledShaderConstant {
    using DefaultValueType = uint64_t;

    const cso::CompiledShaderCollection* pCollection = nullptr;
    const cso::ReflectedConstant* pConstant = nullptr;

    std::string_view Name() const {
        return pConstant ? GetStringViewAtIndex(pCollection, pConstant->name_index()) : EMPTY_STRING;
    }
    std::string_view Macro() const {
        return pConstant ? GetStringViewAtIndex(pCollection, pConstant->macro_name_index()) : EMPTY_STRING;
    }
    size_t Id() const {
        return pConstant ? pConstant->constant_id() : size_t(-1);
        ;
    }
    bool IsSpecialization() const {
        return pConstant && 0 != (pConstant->bits() & cso::ReflectedConstantBit_IsSpecializationBit);
    }
    bool IsUsedAsLUT() const { return pConstant && 0 != (pConstant->bits() & cso::ReflectedConstantBit_IsUsedAsLUT); }
    bool IsUsedAsArrayLength() const {
        return pConstant && 0 != (pConstant->bits() & cso::ReflectedConstantBit_IsUsedAsArrayLengthBit);
    }

    TypeDescription Type() const {
        if (!pConstant) { return {}; }
        if (!pCollection || !pCollection->reflected_types()) { return {}; }
        auto pType = pCollection->reflected_types()->Get(pConstant->type_index());
        return {pCollection, pType};
    }

    template <typename T>
    T DefaultAs() const {
        if (!pConstant) { return {}; }

        if constexpr (sizeof(T) == 1) {
            const uint8_t u = pConstant->default_scalar_u64() & 0xff;
            return reinterpret_cast<const T&>(u);
        }
        if constexpr (sizeof(T) == 2) {
            const uint16_t u = pConstant->default_scalar_u64() & 0xffff;
            return reinterpret_cast<const T&>(u);
        }
        if constexpr (sizeof(T) == 4) {
            const uint32_t u = pConstant->default_scalar_u64() & 0xffffffff;
            return reinterpret_cast<const T&>(u);
        }

        static_assert(sizeof(T) > 8, "Caught unsupported type.");
        const uint64_t u = pConstant->default_scalar_u64();
        return reinterpret_cast<const T&>(u);
    }
};

struct PrecompiledShaderResource {
    const cso::CompiledShaderCollection* pCollection = nullptr;
    const cso::ReflectedResource* pResource = nullptr;
    const cso::ReflectedResourceState* pState = nullptr;

    TypeDescription Type() const {
        if (!pResource) { return {}; }
        if (!pCollection || !pCollection->reflected_types()) { return {}; }
        auto pType = pCollection->reflected_types()->Get(pResource->type_index());
        return {pCollection, pType};
    }

    std::string_view Name() const {
        return pResource ? GetStringViewAtIndex(pCollection, pResource->name_index()) : EMPTY_STRING;
    }

    uint32_t DescriptorSet() const { return pResource ? pResource->descriptor_set() : cso::DecorationValue_Invalid; }
    uint32_t Binding() const { return pResource ? pResource->binding() : cso::DecorationValue_Invalid; }
    uint32_t Location() const { return pResource ? pResource->location() : cso::DecorationValue_Invalid; }
    bool IsActive() const { return pState && pState->is_active(); }
};

struct PrecompiledShaderReflection {
    const cso::CompiledShaderCollection* pCollection = nullptr;
    const cso::ReflectedShader* pReflectedShader = nullptr;

    bool IsValid() const { return pCollection && pReflectedShader; }

    size_t ConstantCount() const {
        return (pReflectedShader && pReflectedShader->constant_indices()) ? pReflectedShader->constant_indices()->size()
                                                                          : 0;
    }
    PrecompiledShaderConstant Constant(size_t index) const {
        if (!pReflectedShader || !pCollection) { return {}; }
        size_t constantIndex = pReflectedShader->constant_indices()->Get(index);
        auto pConstant = pCollection->reflected_constants()->Get(constantIndex);
        return {pCollection, pConstant};
    }

    size_t UniformBufferCount() const {
        return (pReflectedShader && pReflectedShader->uniform_buffer_indices())
                   ? pReflectedShader->uniform_buffer_indices()->size()
                   : 0;
    }
};

struct PrecompiledShaderVariant {
    const cso::CompiledShaderCollection* pCollection = nullptr;
    const cso::CompiledShaderInfo* pCompiledShaderInfo = nullptr;
    const cso::CompiledShader* pCompiledShader = nullptr;
    const cso::ReflectedShader* pReflectedShader = nullptr;

    cso::Shader ShaderType() const { return pCompiledShaderInfo->type(); }
    std::string_view AssetName() const {
        return GetStringViewAtIndex(pCollection, pCompiledShaderInfo->asset_string_index());
    }
    std::string_view AllDefinitions() const {
        return GetStringViewAtIndex(pCollection, pCompiledShaderInfo->definitions_string_index());
    }

    size_t DefinitionCount() const {
        auto indices = pCompiledShaderInfo->definitions_string_indices();
        return indices ? (indices->size() >> 1) : (0);
    }

    ArrayView<const uint8_t> Buffer() const {
        if (!IsCompiled()) { return {}; }
        auto pBuffer = pCollection->buffers()->Get(pCompiledShader->compiled_buffer_index());
        return {pBuffer->contents()->Data(), pBuffer->contents()->size()};
    }

    PrecompiledShaderReflection Reflection() const {
        if (!IsReflected()) { return {}; }
        return {pCollection, pReflectedShader};
    }

    Definition Definition(size_t index) const {
        if (!pCompiledShaderInfo) { return {EMPTY_STRING}; }
        if (!pCompiledShaderInfo->definitions_string_indices() && !index) { return {AllDefinitions()}; }
        if (!pCompiledShaderInfo->definitions_string_indices()) { return {EMPTY_STRING}; }
        if ((index << 1) >= pCompiledShaderInfo->definitions_string_indices()->size()) { return {EMPTY_STRING}; }
        size_t stringIndex = pCompiledShaderInfo->definitions_string_indices()->Get((index << 1) + 0);
        size_t valueIndex = pCompiledShaderInfo->definitions_string_indices()->Get((index << 1) + 1);
        return {GetStringViewAtIndex(pCollection, stringIndex), GetStringViewAtIndex(pCollection, valueIndex)};
    }

    bool IsValid() const { return pCollection && pCompiledShaderInfo; }
    bool IsCompiled() const { return IsValid() && pCompiledShader; }
    bool IsReflected() const { return IsCompiled() && pReflectedShader; }
};

struct PrecompiledShaderLibrary {
    const cso::CompiledShaderCollection* pCollection = nullptr;

    bool IsValid() const { return pCollection != nullptr; }

    PrecompiledShaderVariant GetVariant(const cso::CompiledShaderInfo* pCompiledShaderInfo) const {
        if (!pCompiledShaderInfo) { return {}; }

        auto variant = PrecompiledShaderVariant{pCollection, pCompiledShaderInfo};

        if (pCollection->compiled_shaders()) {
            size_t index = pCompiledShaderInfo->compiled_shader_index();
            variant.pCompiledShader = pCollection->compiled_shaders()->Get(index);
        }

        if (pCollection->reflected_shaders() && variant.pCompiledShader) {
            size_t index = variant.pCompiledShader->reflected_shader_index();
            variant.pReflectedShader = pCollection->reflected_shaders()->Get(index);
        }

        return variant;
    }

    std::pair<PrecompiledShaderVariant, size_t> FindBestMatchWithScore(std::string_view assetName,
                                                                       ArrayView<const Definition> definitions) const {
        if (!pCollection) { return {}; }
        if (!pCollection->compiled_shader_infos()) { return {}; }

        size_t compiledShaderInfoScore = 0;
        const cso::CompiledShaderInfo* pCompiledShaderInfo = nullptr;

        std::for_each(pCollection->compiled_shader_infos()->cbegin(),
                      pCollection->compiled_shader_infos()->cend(),
                      [&](const cso::CompiledShaderInfo* compiledShaderInfo) {
                          if (!compiledShaderInfo) { return; }

                          const auto variant = PrecompiledShaderVariant{pCollection, compiledShaderInfo};
                          if (variant.AssetName() != assetName) { return; };

                          size_t currentScore = 1;
                          size_t definitionCount = variant.DefinitionCount();
                          for (size_t i = 0; i < definitionCount; ++i) {
                              const Definition di = variant.Definition(i);
                              auto it = std::find_if(definitions.begin(), definitions.end(), [&](const Definition& d) {
                                  constexpr int STRING_COMPARE_EQUAL = 0;
                                  return STRING_COMPARE_EQUAL == d.DefinitionName.compare(di.DefinitionName) &&
                                         STRING_COMPARE_EQUAL == d.DefinitionValue.compare(di.DefinitionValue);
                              });

                              currentScore += it != definitions.end();
                          }

                          if (currentScore > compiledShaderInfoScore) {
                              compiledShaderInfoScore = currentScore;
                              pCompiledShaderInfo = compiledShaderInfo;
                          }
                      });

        if (!pCompiledShaderInfo) { return {}; }
        return {GetVariant(pCompiledShaderInfo), compiledShaderInfoScore};
    }

    PrecompiledShaderVariant FindBestMatch(std::string_view assetName, ArrayView<const Definition> definitions) const {
        return FindBestMatchWithScore(assetName, definitions).first;
    }
};

} // namespace cso::utils
