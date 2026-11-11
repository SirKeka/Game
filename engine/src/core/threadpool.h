#include "defines.h"

struct WorkerThread;

class MAPI Threadpool 
{
public:
    bool Create(u32 ThreadCount);
    void Destroy();
    bool Wait();

private:
    u32 ThreadCount;
    struct WorkerThread* threads;
};
