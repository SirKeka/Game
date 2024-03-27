#pragma once

#include "core/logger.hpp"
//#include <new>

class MAPI LinearAllocator
{
public:
    u64 TotalSize{0};
    u64 allocated{0};
    void* memory{nullptr};
    bool OwnsMemory{false};

    static LinearAllocator state;

    LinearAllocator() : TotalSize(0), allocated(0), memory(nullptr), OwnsMemory(false) {}
    LinearAllocator(u64 TotalSize, void* memory = nullptr);
    LinearAllocator& operator= (const LinearAllocator&) = default;
    LinearAllocator& operator= (const LinearAllocator&&);
public:
    ~LinearAllocator();
    LinearAllocator(const LinearAllocator&) = delete;

    void* Allocate(u64 size);
    void FreeAll();

    void Initialize(u64 TotalSize, void* memory = nullptr);
    static LinearAllocator& Instance() { return state; }

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

/*template <typename T>
class WrapLinearAllocator
{

    static LinearAllocator la () {
        static LinearAllocator linearallocator(sizeof(T));
        return linearallocator;
    }
public:
    static void* Allocate(u64 size) { return la().Allocate(size); }
    static void  Free(void* ptr) { la().FreeAll; }

};*/
