#pragma once

#include <cstdint>
#include <functional>
#include <string_view>
#include <type_traits>

namespace apemode {
uint64_t CityHash64(const char *buf, uint64_t len);
uint64_t CityHash64(std::string_view sv);

template <typename T, bool IsIntegral = std::is_integral<T>::value>
uint64_t CityHash64(T const &Obj) {
    static_assert(!std::is_polymorphic_v<T>, "Caught invalid call.");
    if constexpr (std::is_integral<T>::value) { return static_cast<uint64_t>(Obj); }
    return apemode::CityHash64(reinterpret_cast<const char *>(&Obj), sizeof(T));
}

uint64_t CityHash128to64(uint64_t a, uint64_t b);

template <typename T>
uint64_t CityHash128to64(uint64_t a, T const &Obj) {
    if (a) { return CityHash128to64(a, CityHash64(Obj)); }
    return CityHash64(Obj);
}

struct CityHasher64 {
    uint64_t Value;

    CityHasher64() : Value(0xc949d7c7509e6557) {}
    CityHasher64(uint64_t OtherHash) : Value(OtherHash) {}

    void CombineWith(CityHasher64 const &Other) { Value = CityHash128to64(Value, Other.Value); }

    template <typename T>
    void CombineWith(T const &Pod) {
        static_assert(!std::is_polymorphic_v<T>, "Caught invalid call.");
        Value = CityHash128to64(Value, Pod);
    }

    void CombineWithBuffer(const void *pBuffer, size_t bufferSize) {
        const auto pBytes = reinterpret_cast<char const *>(pBuffer);
        const auto BufferHash = CityHash64(pBytes, bufferSize);
        Value = CityHash128to64(Value, BufferHash);
    }

    template <typename T>
    void CombineWithArray(T const *pStructs, size_t count) {
        static_assert(!std::is_polymorphic_v<T>, "Caught invalid call.");
        constexpr size_t stride = sizeof(T);
        CombineWithBuffer(pStructs, count * stride);
    }

    operator uint64_t() const { return Value; }
};
} // namespace apemode
