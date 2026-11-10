#pragma once

#include "defines.h"

struct MAPI Semaphore 
{
    void *InternalData;

    bool Create(u32 MaxCount, u32 StartCount);
    void Destroy();

    bool Signal();

    /// @brief Уменьшает значение семафора на 1. Если значение достигает 0, 
    /// семафор считается перешедшим в несигнальное состояние, 
    /// и данный вызов блокируется до тех пор, 
    /// пока семафор не будет переведен в сигнальное состояние с помощью Signal().
    bool Wait(u64 TimeoutMs);
};
