#include "defines.h"

class MAPI Threadpool 
{
public:
    //ЗАДАЧА: убрать данные класса в приватную зону и сделать функции обертки?

    u32 ThreadCount;
    struct WorkerThread* threads;

    constexpr Threadpool(u32 ThreadCount, bool& result);
    ~Threadpool() { Destroy(); }

    bool Create(u32 ThreadCount);
    void Destroy();
    bool Wait();
};
