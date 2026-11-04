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
        u64 addr = reinterpret_cast<u64>(block);
        addr += (ElementSize * index);
        return reinterpret_cast<void*>(addr);
    };

    void* pivot = DataAtIndex(data, size, HighIndex);

    i32 i = (LowIndex - 1);

    for (i32 j = LowIndex; j <= HighIndex - 1; ++j) {
        void* dataj = DataAtIndex(data, size, j);
        i32 result = ComparePfn(dataj, pivot);
        if (result > 0) {
            ++i;
            void* datai = DataAtIndex(data, size, i);
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
        void* ScratchMem = MemorySystem::AllocateTemp(TypeSize, false);
        i32 PartitionIndex = QuickSortPartition(ScratchMem, TypeSize, data, LowIndex, HighIndex, ComparePfn);
        QuickSortInternal(ScratchMem, TypeSize, data, LowIndex, PartitionIndex - 1, ComparePfn);
        QuickSortInternal(ScratchMem, TypeSize, data, PartitionIndex + 1, HighIndex, ComparePfn);
    }
}

// void Moon::BubleSort(u64 TypeSize, void *data, i32 LowIndex, i32 HighIndex, PFN_QuicksortCompare ComparePfn)
// {
//     if (LowIndex < HighIndex) {
//         for (u64 i = 0; i < count; i++) {
//             i32 result = ComparePfn(data + i, data + i + 1);
// 
//             if (result > 0) {
//                 void* ScratchMem = AllocateTemp<void>(TypeSize, false);
//                 PtrSwap(ScratchMem, TypeSize, data + i, data + i + 1);
//             } else if (result == 0) {
//                 
//             }
//             
//         }
//     }
// }
