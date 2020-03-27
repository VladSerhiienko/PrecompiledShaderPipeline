#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace apemode {
namespace platform {

struct IAsset {
    virtual ~IAsset() = default;
    virtual const char* GetName() const = 0;
    virtual const char* GetId() const = 0;
    virtual std::vector<uint8_t> GetContentAsTextBuffer() const = 0;
    virtual std::vector<uint8_t> GetContentAsBinaryBuffer() const = 0;
    virtual uint64_t GetCurrentVersion() const = 0;
    virtual void SetCurrentVersion(uint64_t) = 0;
};

struct IAssetManager {
    virtual ~IAssetManager() = default;
    virtual const IAsset* Acquire(const char* pszAssetName) const = 0;
    virtual void Release(const IAsset* pAsset) const = 0;
};

std::unique_ptr<IAssetManager> CreateAssetManager();

} // namespace platform
} // namespace apemode
