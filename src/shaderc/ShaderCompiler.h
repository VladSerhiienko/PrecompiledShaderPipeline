#pragma once

#include <cassert>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace apemode {
namespace shp {

enum class ReflectedPrimitiveType {
    Struct,
    Bool,
    Char,
    Short,
    Int,
    Long,
    UChar,
    UShort,
    UInt,
    ULong,
    Half,
    Float,
    Double,
    Image,
    Sampler,
    SampledImage,
    AtomicCounter,
    AccelerationStructure,
    ControlPointArray,
    Error,
};

struct ReflectedStructMember;
struct ReflectedType {
    ReflectedType();
    ~ReflectedType();

    std::string Name = "";
    ReflectedPrimitiveType ElementPrimitiveType = ReflectedPrimitiveType::Error;
    uint32_t ElementByteSize = 0;
    uint32_t ElementVectorLength = 0;
    uint32_t ElementColumnCount = 0;
    uint32_t ElementMatrixByteStride = 0;
    uint32_t ArrayLength = 0;
    bool bIsArrayLengthStatic = false;
    uint32_t ArrayByteStride = 0;
    uint32_t EffectiveByteSize = 0;
    std::vector<std::shared_ptr<ReflectedStructMember>> Members = {};
};

struct ReflectedStructMember {
    std::string Name = "";
    ReflectedType Type = {};
    uint32_t EffectiveByteSize = 0;
    uint32_t OccupiedByteSize = 0;
    uint32_t ByteOffset = 0;
};

struct ReflectedMemoryRange {
    uint32_t offset = 0;
    uint32_t size = 0;
};

struct ReflectedResource {
    std::string Name = "";
    ReflectedType Type = {};
    uint32_t DecorationDescriptorSet = -1;
    uint32_t DecorationBinding = -1;
    uint32_t DecorationLocation = -1;
    bool bIsActive = false;
    std::vector<ReflectedMemoryRange> ActiveRanges = {};
};

struct ReflectedConstantDefaultValue {
    union {
        uint8_t u8[8];
        uint64_t u64;
        double d;
    };
};

static_assert(sizeof(ReflectedConstantDefaultValue) == 8, "Caught size mismatch.");

struct ReflectedConstant {
    std::string Name = "";
    std::string MacroName = "";
    ReflectedConstantDefaultValue DefaultValue = {};
    uint32_t ConstantId = -1;
    ReflectedType Type = {};
    bool bIsUsedAsLUT = false;
    bool bIsUsedAsArrayLength = false;
    bool bIsSpecialization = false;
};

struct ReflectedShader {
    std::string Name = "";
    std::vector<ReflectedConstant> Constants = {};
    std::vector<ReflectedResource> StageInputs = {};
    std::vector<ReflectedResource> StageOutputs = {};
    std::vector<ReflectedResource> UniformBuffers = {};
    std::vector<ReflectedResource> PushConstantBuffers = {};
    std::vector<ReflectedResource> SampledImages = {};
    std::vector<ReflectedResource> SubpassInputs = {};
    std::vector<ReflectedResource> StorageImages = {};
    std::vector<ReflectedResource> StorageBuffers = {};
    std::vector<ReflectedResource> SeparateImages = {};
    std::vector<ReflectedResource> SeparateSamplers = {};
};

enum class CompiledShaderTarget { Preprocessed = 0, SpvAssembly, VulkanGLSL, ES2GLSL, ES3GLSL, iOSMTL, macOSMTL, HLSL, Count };

class ICompiledShader {
public:
    virtual ~ICompiledShader() = default;

    virtual const uint8_t* GetBytePtr() const = 0;
    virtual size_t GetByteCount() const = 0;
    virtual std::string_view GetSourceFor(CompiledShaderTarget target) const = 0;
    virtual std::string_view GetErrorFor(CompiledShaderTarget target) const = 0;
    virtual bool HasSourceFor(CompiledShaderTarget target) const = 0;
    virtual const ReflectedShader& GetReflection() const = 0;

    // clang-format off
    inline const uint32_t* GetDwordPtr() const { return reinterpret_cast<const uint32_t*>(GetBytePtr()); }
    inline size_t GetDwordCount() const { assert(GetByteCount() % 4 == 0); return GetByteCount() >> 2; }
    // clang-format on
};

/* Wrapper for shaderc lib */
class IShaderCompiler {
public:
    struct MacroDefinition {
        const char* pszKey = nullptr;
        const char* pszValue = nullptr;
    };

    class IMacroDefinitionCollection {
    public:
        virtual ~IMacroDefinitionCollection() = default;
        virtual size_t GetCount() const = 0;
        virtual MacroDefinition GetMacroDefinition(size_t macroIndex) const = 0;
    };

    /* Simple interface to insert included files */
    class IIncludedFileSet {
    public:
        virtual ~IIncludedFileSet() = default;
        virtual void InsertIncludedFile(const std::string& includedFileName) = 0;
    };

    /* Simple interface to read files from shader assets */
    class IShaderFileReader {
    public:
        virtual ~IShaderFileReader() = default;
        // virtual bool FileExists(const std::string& filePath) = 0;

        virtual bool ReadShaderTxtFile(const std::string& filePath,
                                       std::string& OutFileFullPath,
                                       std::string& OutFileContent,
                                       bool bRelative) = 0;
    };

    /* Simple interface to read files from shader assets */
    class IShaderFeedbackWriter {
    public:
        enum EFeedbackTypeBits {
            /* Copy-paste from shaderc */
            eFeedbackType_CompilationStatus_Success = 0,
            eFeedbackType_CompilationStatus_InvalidStage, /* Error stage deduction */
            eFeedbackType_CompilationStatus_CompilationError,
            eFeedbackType_CompilationStatus_InternalError, /* Unexpected failure */
            eFeedbackType_CompilationStatus_NullResultObject,
            eFeedbackType_CompilationStatus_InvalidAssembly,
            eFeedbackType_CompilationStatusMask = 0xf,

            /* Compilation Stage */
            eFeedbackType_CompilationStage_Preprocessed = 0x10,          /* Txt */
            eFeedbackType_CompilationStage_PreprocessedOptimized = 0x20, /* Txt */
            eFeedbackType_CompilationStage_Assembly = 0x30,              /* Txt */
            eFeedbackType_CompilationStage_Spv = 0x40,                   /* Bin */
            eFeedbackType_CompilationStageMask = 0xf0,
        };

        using EFeedbackType = std::underlying_type<EFeedbackTypeBits>::type;

        // clang-format off
        static const EFeedbackType eFeedback_PreprocessingSucceeded = eFeedbackType_CompilationStage_Preprocessed | eFeedbackType_CompilationStatus_Success;
        static const EFeedbackType eFeedback_OptimizationSucceeded = eFeedbackType_CompilationStage_PreprocessedOptimized | eFeedbackType_CompilationStatus_Success;
        static const EFeedbackType eFeedback_AssemblySucceeded = eFeedbackType_CompilationStage_PreprocessedOptimized | eFeedbackType_CompilationStatus_Success;
        static const EFeedbackType eFeedback_SpvSucceeded = eFeedbackType_CompilationStage_PreprocessedOptimized | eFeedbackType_CompilationStatus_Success;
        // clang-format on

        /**
         * If CompilationStatus bit is not Success, pContent is an error message (Txt).
         * pContent is Bin in case CompilationStatus bit is success and Stage is Spv.
         **/
        virtual void WriteFeedback(EFeedbackType eType,
                                   const std::string& fullFilePath,
                                   const IMacroDefinitionCollection* pMacros,
                                   const void* pContent, /* Txt or bin, @see EFeedbackType */
                                   const void* pContentEnd) = 0;
    };

    /* Copy-paste from shaderc */
    enum class ShaderType {
        Vertex = 0,
        Fragment,
        Compute,
        Geometry,
        TessControl,
        TessEvaluation,
        RayGeneration,
        AnyHit,
        ClosestHit,
        Miss,
        Intersection,
        Callable,
        Task,
        Mesh,
        Count,
    };

    /* Copy-paste from shaderc */
    enum ShaderOptimizationType {
        None,        // no optimization
        Size,        // optimize towards reducing code size
        Performance, // optimize towards performance
    };

    virtual ~IShaderCompiler() = default;

    /* @note Compiling from source string */

    virtual std::unique_ptr<ICompiledShader> Compile(const std::string& shaderName,
                                                     const std::string& sourceCode,
                                                     const IMacroDefinitionCollection* pMacros,
                                                     ShaderType shaderType,
                                                     ShaderOptimizationType optimizationType) const = 0;

    /* @note Compiling from source files */

    virtual IShaderFileReader* GetShaderFileReader() = 0;
    virtual IShaderFeedbackWriter* GetShaderFeedbackWriter() = 0;
    virtual void SetShaderFileReader(IShaderFileReader* pShaderFileReader) = 0;
    virtual void SetShaderFeedbackWriter(IShaderFeedbackWriter* pShaderFeedbackWriter) = 0;

    virtual std::unique_ptr<ICompiledShader> Compile(const std::string& filePath,
                                                     const IMacroDefinitionCollection* pMacros,
                                                     ShaderType shaderType,
                                                     ShaderOptimizationType optimizationType,
                                                     IIncludedFileSet* pOutIncludedFiles) const = 0;
};

std::unique_ptr<IShaderCompiler> NewShaderCompiler();

} // namespace shp
} // namespace apemode
