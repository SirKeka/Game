#include "sort.h"
#include "core/memory_system.h"

template<typename T>
void Swap(T& a, T& b) {
    auto temp = (T&&)a;
    a = (T&&)b;
    b = (T&&)temp;
}

void Moon::PtrSwap(void *ScratchMem, u64 size, void *a, void *b)
{
    MemorySystem::CopyMem(ScratchMem, a, size);
    MemorySystem::CopyMem(a, b, size);
    MemorySystem::CopyMem(b, ScratchMem, size);
}

static i32 QuickSortPartition(void* ScratchMem, u64 size, void* data, i32 LowIndex, i32 HighIndex, PFN_QuicksortCompare ComparePfn) {
    auto DataAtIndex = [](void* block, u64 ElementSize, u32 index) {
        return (void*)(((u64)block) + (ElementSize * index));
    };

    void* pivot = DataAtIndex(data, size, HighIndex);

    i32 i = (LowIndex - 1);
    void* datai = DataAtIndex(data, size, i);

    for (i32 j = LowIndex; j <= HighIndex - 1; ++j) {
        void* dataj = DataAtIndex(data, size, j);
        i32 result = ComparePfn(dataj, pivot);
        if (result < 0) {
            Moon::PtrSwap(ScratchMem, size, datai, dataj);
        } else if (result > 0) {
            Moon::PtrSwap(ScratchMem, size, dataj, datai);
        }
    }
    Moon::PtrSwap(ScratchMem, size, DataAtIndex(data, size, i + 1), DataAtIndex(data, size, HighIndex));
    return i + 1;
}

static void QuickSortInternal(void* ScratchMem, u64 size, void* data, i32 LowIndex, i32 HighIndex, PFN_QuicksortCompare ComparePfn) {
    if (LowIndex < HighIndex) {
        i32 PartitionIndex = QuickSortPartition(ScratchMem, size, data, LowIndex, HighIndex, ComparePfn);
        QuickSortInternal(ScratchMem, size, data, LowIndex, PartitionIndex - 1, ComparePfn);
        QuickSortInternal(ScratchMem, size, data, PartitionIndex + 1, HighIndex, ComparePfn);
    }
}

void Moon::QuickSort(u64 TypeSize, void *data, i32 LowIndex, i32 HighIndex, PFN_QuicksortCompare ComparePfn)
{
    if (LowIndex < HighIndex) {
        void* ScratchMem = MemorySystem::Allocate(TypeSize, Memory::Array);
        i32 PartitionIndex = QuickSortPartition(ScratchMem, TypeSize, data, LowIndex, HighIndex, ComparePfn);
        QuickSortInternal(ScratchMem, TypeSize, data, LowIndex, PartitionIndex - 1, ComparePfn);
        QuickSortInternal(ScratchMem, TypeSize, data, PartitionIndex + 1, HighIndex, ComparePfn);
        MemorySystem::Free(ScratchMem, TypeSize, Memory::Array);
    }
}
