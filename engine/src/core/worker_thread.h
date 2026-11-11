#pragma once

#include "mthread.h"
#include "containers/queue.h"
#include "mmutex.h"

struct MAPI WorkerThread 
{
    MThread thread;
    Queue WorkQueue;
    MMutex QueueMutex;

    /// @brief 
    constexpr WorkerThread();

    bool Create();
    void Destroy();
    bool Add(PFN_ThreadStart WorkFn, void* params);
    bool Start();
    bool Wait();
};
