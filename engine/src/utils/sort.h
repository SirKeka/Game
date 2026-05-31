#pragma once

#include "defines.h"

using PFN_QuicksortCompare = i32 (*)(void* a, void* b);

namespace Moon {
    MAPI void PtrSwap(void* ScratchMem, u64 size, void* a, void* b);

    MAPI void QuickSort(u64 TypeSize, void* data, i32 LowIndex, i32 HighIndex, PFN_QuicksortCompare ComparePfn);
    
} // namespace Moon



