#pragma once

#include "core/logger.hpp"

class MAPI LinearAllocator
{
public:
    u64 TotalSize{0};
    u64 allocated{0};
    void* memory{nullptr};
    bool OwnsMemory{false};
public:
    constexpr LinearAllocator() : TotalSize(0), allocated(0), memory(nullptr), OwnsMemory(false) {}
    constexpr LinearAllocator(u64 TotalSize, void* memory = nullptr);

    void Initialize(u64 TotalSize, void* memory = nullptr);

    ~LinearAllocator();
    LinearAllocator(const LinearAllocator&) = delete;

    void* Allocate(u64 size);
    void FreeAll();

    /// @brief 
    /// @tparam T 
    /// @tparam ...Args 
    /// @param ptr 
    /// @param ...args 
    /// @return 
    template<class T, class... Args>
    MINLINE T AllocateConstruct(u64 byte, T* value, Args && ...args) {
        if (void* mem = Allocate<T>(byte)) {
        return new(mem) T(args...);
    }
    return {};
    }
};
