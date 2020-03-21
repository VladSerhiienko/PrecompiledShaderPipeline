
#include <apemode/platform/AppState.h>
#include <apemode/platform/CityHash.h>
#include <apemode/platform/shared/AssetManager.h>
#include <flatbuffers/util.h>

#include <nlohmann/json.hpp>

#include "ShaderCompiler.Vulkan.h"
#include "cso_generated.h"

using json = nlohmann::json;

namespace {
static constexpr unsigned int crc_table[256] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3, 0x0edb8832,
    0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7, 0x136c9856, 0x646ba8c0, 0xfd62f97a,
    0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3,
    0x45df5c75, 0xdcd60dcf, 0xabd13d59, 0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab,
    0xb6662d3d, 0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01, 0x6b6b51f4,
    0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65, 0x4db26158, 0x3ab551ce, 0xa3bc0074,
    0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525,
    0x206f85b3, 0xb966d409, 0xce61e49f, 0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615,
    0x73dc1683, 0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7, 0xfed41b76,
    0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b, 0xd80d2bda, 0xaf0a1b4c, 0x36034af6,
    0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7,
    0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d, 0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7,
    0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45, 0xa00ae278,
    0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9, 0xbdbdf21c, 0xcabac28a, 0x53b39330,
    0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d};

constexpr uint32_t CRC32(std::string_view str) {
    uint32_t crc = 0xffffffff;
    for (auto c : str) crc = (crc >> 8) ^ crc_table[(crc ^ c) & 0xff];
    return crc ^ 0xffffffff;
}

constexpr uint32_t Rotate32(uint32_t val, int shift) {
    // Avoid shifting by 32: doing so yields an undefined result.
    return shift == 0 ? val : ((val >> shift) | (val << (32 - shift)));
}

constexpr uint32_t Mur(uint32_t a, uint32_t h) {
    constexpr uint32_t c1 = 0xcc9e2d51;
    constexpr uint32_t c2 = 0x1b873593;

    // Helper from Murmur3 for combining two 32-bit values.
    a *= c1;
    a = Rotate32(a, 17);
    a *= c2;
    h ^= a;
    h = Rotate32(h, 19);
    return h * 5 + 0xe6546b64;
}

class HashedString {
public:
    std::string Contents = "";
    uint32_t Hash = 0;
};

class HashedBuffer {
public:
    std::vector<uint8_t> Contents = {};
    uint32_t Hash = 0;
};

class CompiledShaderVariant {
public:
    std::string Preprocessed = "";
    std::string Assembly = "";
    std::string Asset = "";
    std::string Definitions = "";
    std::string CompiledGLSL = "";
    std::string CompiledMSL = "";
    std::vector<uint8_t> Compiled = {};
    cso::EShader Type = cso::EShader::EShader_MAX;
    std::set<std::string> IncludedFiles;
    std::map<std::string, std::string> DefinitionMap;
};

class CompiledShader {
public:
    uint32_t PreprocessedIndex = 0;
    uint32_t AssemblyIndex = 0;
    uint32_t CompiledIndex = 0;
    uint32_t GLSLIndex = 0;
    uint32_t MSLIndex = 0;
    uint32_t Hash = 0;
};

class CompiledShaderInfo {
public:
    uint32_t AssetIndex = 0;
    uint32_t CompiledShaderIndex = 0;
    uint32_t DefinitionsIndex = 0;
    cso::EShader Type = cso::EShader::EShader_MAX;
    std::vector<uint32_t> IncludedFileIndices;
    std::vector<uint32_t> DefinitionIndices;
    uint32_t Hash = 0;
};

class CompiledShaderCollection {
public:
    std::vector<CompiledShader> compiledShaders = {};
    std::vector<CompiledShaderInfo> compiledShaderInfos = {};
    std::vector<HashedString> strings = {};
    std::vector<HashedBuffer> buffers = {};

    void Serialize(flatbuffers::FlatBufferBuilder& fbb, const std::vector<CompiledShaderVariant>& variants) {
        Pack(variants);

        std::vector<flatbuffers::Offset<cso::HashedBuffer>> hashedBufferOffsets = {};
        for (auto& hashedBuffer : buffers) {
            const int8_t* contentsPtr = (const int8_t*)hashedBuffer.Contents.data();
            const uint32_t contentsLen = hashedBuffer.Contents.size();
            flatbuffers::Offset<flatbuffers::Vector<int8_t>> contentsOffset =
                fbb.CreateVector(contentsPtr, contentsLen);
            hashedBufferOffsets.push_back(cso::CreateHashedBuffer(fbb, contentsOffset, hashedBuffer.Hash));
            apemode::LogInfo("+ Buffer -> {}", contentsLen);
        }

        flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<cso::HashedBuffer>>> hashedBuffersOffset =
            fbb.CreateVector(hashedBufferOffsets.data(), hashedBufferOffsets.size());

        std::vector<flatbuffers::Offset<cso::HashedString>> hashedStringOffsets = {};
        for (auto& hashedString : strings) {
            const char* contentsPtr = hashedString.Contents.c_str();
            const uint32_t contentsLen = hashedString.Contents.length();
            flatbuffers::Offset<flatbuffers::String> contentsOffset = fbb.CreateString(contentsPtr, contentsLen);
            hashedStringOffsets.push_back(cso::CreateHashedString(fbb, contentsOffset, hashedString.Hash));
            apemode::LogInfo("+ String -> {}", contentsLen);
        }

        flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<cso::HashedString>>> hashedStringsOffset =
            fbb.CreateVector(hashedStringOffsets.data(), hashedStringOffsets.size());

        std::vector<cso::CompiledShader> compiledShaderOffsets = {};
        for (auto& compiledShader : compiledShaders) {
            compiledShaderOffsets.push_back(cso::CompiledShader(compiledShader.PreprocessedIndex,
                                                                compiledShader.AssemblyIndex,
                                                                compiledShader.CompiledIndex,
                                                                compiledShader.GLSLIndex,
                                                                compiledShader.MSLIndex,
                                                                -1,
                                                                cso::EIR_SPIRV,
                                                                compiledShader.Hash));
            apemode::LogInfo("+ CSO -> {}", compiledShader.CompiledIndex);
        }

        flatbuffers::Offset<flatbuffers::Vector<const cso::CompiledShader*>> compiledShadersOffset =
            fbb.CreateVectorOfStructs(compiledShaderOffsets.data(), compiledShaderOffsets.size());

        std::vector<flatbuffers::Offset<cso::CompiledShaderInfo>> compiledShaderInfoOffsets = {};
        for (auto& compiledShaderInfo : compiledShaderInfos) {
            flatbuffers::Offset<flatbuffers::Vector<uint32_t>> includedFilesOffset = fbb.CreateVector(
                compiledShaderInfo.IncludedFileIndices.data(), compiledShaderInfo.IncludedFileIndices.size());
            flatbuffers::Offset<flatbuffers::Vector<uint32_t>> definitionsOffset = fbb.CreateVector(
                compiledShaderInfo.DefinitionIndices.data(), compiledShaderInfo.DefinitionIndices.size());
            compiledShaderInfoOffsets.push_back(cso::CreateCompiledShaderInfo(fbb,
                                                                              cso::EShader(compiledShaderInfo.Type),
                                                                              compiledShaderInfo.CompiledShaderIndex,
                                                                              compiledShaderInfo.AssetIndex,
                                                                              compiledShaderInfo.DefinitionsIndex,
                                                                              definitionsOffset,
                                                                              includedFilesOffset,
                                                                              compiledShaderInfo.Hash));
            apemode::LogInfo("+ CSO Info -> {}", compiledShaderInfo.CompiledShaderIndex);
        }

        flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<cso::CompiledShaderInfo>>>
            compiledShaderInfosOffset =
                fbb.CreateVector(compiledShaderInfoOffsets.data(), compiledShaderInfoOffsets.size());

        flatbuffers::Offset<cso::CompiledShaderCollection> collectionOffset =
            cso::CreateCompiledShaderCollection(fbb,
                                                cso::EVersion_Value,
                                                compiledShaderInfosOffset,
                                                compiledShadersOffset,
                                                0,
                                                0,
                                                0,
                                                0,
                                                hashedStringsOffset,
                                                hashedBuffersOffset);

        FinishCompiledShaderCollectionBuffer(fbb, collectionOffset);
    }

    void Pack(const std::vector<CompiledShaderVariant>& variants) {
        for (auto& cso : variants) {
            CompiledShader compiledShader = {};
            compiledShader.CompiledIndex = GetBufferIndex(cso.Compiled);
            compiledShader.GLSLIndex = GetStringIndex(cso.CompiledGLSL);
            compiledShader.MSLIndex = GetStringIndex(cso.CompiledMSL);
            compiledShader.AssemblyIndex = GetStringIndex(cso.Assembly);
            compiledShader.PreprocessedIndex = GetStringIndex(cso.Preprocessed);
            compiledShader.Hash = buffers[compiledShader.CompiledIndex].Hash;

            uint32_t compiledShaderIndex = GetCompiledShaderIndex(compiledShader);

            apemode::CityHash64Wrapper city64;
            CompiledShaderInfo compiledShaderInfo = {};

            compiledShaderInfo.CompiledShaderIndex = compiledShaderIndex;
            compiledShaderInfo.AssetIndex = GetStringIndex(cso.Asset);
            compiledShaderInfo.Type = cso.Type;
            compiledShaderInfo.DefinitionsIndex = GetStringIndex(cso.Definitions);

            city64.CombineWith(compiledShaderInfo.CompiledShaderIndex);
            city64.CombineWith(compiledShaderInfo.AssetIndex);
            city64.CombineWith(compiledShaderInfo.Type);
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

            union {
                uint64_t u64;
                struct {
                    uint32_t u32_0;
                    uint32_t u32_1;
                };
            } components;

            components.u64 = city64.Value;
            compiledShaderInfo.Hash = Mur(components.u32_0, components.u32_1);

            AddCompiledShaderInfoIndex(compiledShaderInfo);
        }
    }

    // clang-format off
    void AddCompiledShaderInfoIndex(const CompiledShaderInfo& compiledShaderInfo) {
        auto it = std::find_if(compiledShaderInfos.begin(), compiledShaderInfos.end(), [compiledShaderInfo](const CompiledShaderInfo& existingCompiledShaderInfo) { return existingCompiledShaderInfo.Hash == compiledShaderInfo.Hash; });
        if (it == compiledShaderInfos.end()) { compiledShaderInfos.push_back(compiledShaderInfo); }
    }
    uint32_t GetCompiledShaderIndex(const CompiledShader& compiledShader) {
        auto it = std::find_if(compiledShaders.begin(), compiledShaders.end(), [compiledShader](const CompiledShader& existingCompiledShader) { return existingCompiledShader.Hash == compiledShader.Hash; });
        if (it != compiledShaders.end()) { return std::distance(compiledShaders.begin(), it); }
        uint32_t index = compiledShaders.size();
        compiledShaders.push_back(compiledShader);
        return index;
    }
    uint32_t GetStringIndex(const std::string& string) {
        uint32_t hash = CRC32(string);
        auto it = std::find_if(strings.begin(), strings.end(), [hash](const HashedString& hashedString) { return hashedString.Hash == hash; });
        if (it != strings.end()) { return std::distance(strings.begin(), it); }
        uint32_t index = strings.size();
        strings.push_back({string, hash});
        return index;
    }
    uint32_t GetBufferIndex(const std::vector<uint8_t>& buffer) {
        uint32_t hash = CRC32(std::string_view{(const char*)buffer.data(), buffer.size()});
        auto it = std::find_if(buffers.begin(), buffers.end(), [hash](const HashedBuffer& hashedBuffer) { return hashedBuffer.Hash == hash; });
        if (it != buffers.end()) { return std::distance(buffers.begin(), it); }
        uint32_t index = buffers.size();
        buffers.push_back({buffer, hash});
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
        apemode_memory_allocation_scope;
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

struct EFeedbackTypeWithOStream {
    const apemode::shp::IShaderCompiler::IShaderFeedbackWriter::EFeedbackType e;

    EFeedbackTypeWithOStream(const apemode::shp::IShaderCompiler::IShaderFeedbackWriter::EFeedbackType e) : e(e) {}

    // clang-format off
    template < typename OStream >
    inline friend OStream& operator<<( OStream& os, const EFeedbackTypeWithOStream& feedbackType ) {
        using e = apemode::shp::IShaderCompiler::IShaderFeedbackWriter;
        switch ( feedbackType.e ) {
        case e::eFeedbackType_CompilationStage_Assembly:                 return os << "Assembly";
        case e::eFeedbackType_CompilationStage_Preprocessed:             return os << "Preprocessed";
        case e::eFeedbackType_CompilationStage_PreprocessedOptimized:    return os << "PreprocessedOptimized";
        case e::eFeedbackType_CompilationStage_Spv:                      return os << "Spv";
        case e::eFeedbackType_CompilationStatus_CompilationError:        return os << "CompilationError";
        case e::eFeedbackType_CompilationStatus_InternalError:           return os << "InternalError";
        case e::eFeedbackType_CompilationStatus_InvalidAssembly:         return os << "InvalidAssembly";
        case e::eFeedbackType_CompilationStatus_InvalidStage:            return os << "InvalidStage";
        case e::eFeedbackType_CompilationStatus_NullResultObject:        return os << "NullResultObject";
        case e::eFeedbackType_CompilationStatus_Success:                 return os << "Success";
        default:                                                                                                            return os;
        }
    }
    // clang-format on
};

class ShaderFeedbackWriter : public apemode::shp::IShaderCompiler::IShaderFeedbackWriter {
public:
    void WriteFeedback(EFeedbackType eType,
                       const std::string& FullFilePath,
                       const apemode::shp::IShaderCompiler::IMacroDefinitionCollection* pMacros,
                       const void* pContent,
                       const void* pContentEnd) {
        apemode_memory_allocation_scope;

        const auto feedbackStage = eType & eFeedbackType_CompilationStageMask;
        const auto feedbackCompilationError = eType & eFeedbackType_CompilationStatusMask;

        if (eFeedbackType_CompilationStatus_Success != feedbackCompilationError) {
            apemode::LogError("ShaderCompiler: {}/{}: {}",
                              EFeedbackTypeWithOStream(feedbackStage),
                              EFeedbackTypeWithOStream(feedbackCompilationError),
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

apemode::shp::IShaderCompiler::EShaderType GetShaderType(const std::string& shaderType) {
    if (shaderType == "vert") {
        return apemode::shp::IShaderCompiler::eShaderType_VertexShader;
    } else if (shaderType == "frag") {
        return apemode::shp::IShaderCompiler::eShaderType_FragmentShader;
    } else if (shaderType == "comp") {
        return apemode::shp::IShaderCompiler::eShaderType_ComputeShader;
    } else if (shaderType == "geom") {
        return apemode::shp::IShaderCompiler::eShaderType_GeometryShader;
    } else if (shaderType == "tesc") {
        return apemode::shp::IShaderCompiler::eShaderType_TessControlShader;
    } else if (shaderType == "tese") {
        return apemode::shp::IShaderCompiler::eShaderType_TessEvaluationShader;
    }

    apemode::LogError("Shader type should be one of there: vert, frag, comp, tesc, tese, instead got value: {}",
                      shaderType);
    return apemode::shp::IShaderCompiler::eShaderType_GLSL_InferFromSource;
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
    CompiledShaderVariant cso = {};

    std::string macrosString = GetMacrosString(macroDefinitions);
    cso.Definitions = macrosString;

    ShaderCompilerMacroDefinitionCollection concreteMacros;
    concreteMacros.Init(macroDefinitions);

    apemode::shp::IShaderCompiler::EShaderType eShaderType = GetShaderType(shaderType);
    cso.Type = cso::EShader(eShaderType);

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
        cso.IncludedFiles = includedFileSet.IncludedFiles;
        cso.DefinitionMap = macroDefinitions;
        cso.Definitions = macrosString;
        cso.Asset = srcFile;
        cso.Type = cso::EShader(eShaderType);

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

        apemode::LogInfo("Variant: asset=\"{}\", definitions=\"{}\"", cso.Asset, cso.Definitions);
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
            csos.push_back(cso.value());
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
                csos.push_back(cso.value());
            }
        }

        return csos;
    }

    apemode::LogError("Asset not found, the command \"{}\" skipped.", commandJson.dump().c_str());
    return {};
}
} // namespace

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
        compiledShaders.insert(compiledShaders.end(), variants.begin(), variants.end());
    }
    // clang-format on

    CompiledShaderCollection collection;
    flatbuffers::FlatBufferBuilder fbb;
    collection.Serialize(fbb, compiledShaders);

    flatbuffers::Verifier v(fbb.GetBufferPointer(), fbb.GetSize());
    assert(cso::VerifyCompiledShaderCollectionBuffer(v));

    auto builtBuffePtr = (const char*)fbb.GetBufferPointer();
    auto builtBuffeLen = (size_t)fbb.GetSize();

    apemode::LogInfo("CSO file: {}", outputFile);
    if (!flatbuffers::SaveFile(outputFile.c_str(), builtBuffePtr, builtBuffeLen, true)) {
        apemode::LogError("Failed to write CSO ({} bytes) to file: '{}'", builtBuffeLen, outputFile);
        return 1;
    }

    apemode::LogInfo("Done.");
    return 0;
}
