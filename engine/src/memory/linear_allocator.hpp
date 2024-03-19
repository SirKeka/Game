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
public:
    LinearAllocator() : TotalSize(0), allocated(0), memory(nullptr), OwnsMemory(false) {}
    LinearAllocator(u64 TotalSize, void* memory = nullptr);
    ~LinearAllocator();

    void* Allocate(u64 size) {
        if (memory) {
            if (allocated + size > TotalSize) {
                u64 remaining = TotalSize - allocated;
                MERROR("LinearAllocator::AllocateConstruct - Попытка выделить %llu байт, только %llu байт осталось.", size, remaining);
                return nullptr;
            }

            void* block = reinterpret_cast<u8*>(memory) + allocated;
            allocated += size;
            return block;
        }

        MERROR("Linear Allocator::Allocate - предоставленный распределитель не инициализирован.");
        return nullptr;
    }
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
    void FreeAll();
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
