#include "threadpool.h"
#include "memory_system.h"
#include "worker_thread.h"

bool Threadpool::Create(u32 ThreadCount)
{
    if (!ThreadCount) {
        MERROR("Для работы Threadpool::Create требуется как минимум 1 поток, а также корректный указатель для сохранения созданного пула.");
        return false;
    }

    this->ThreadCount = ThreadCount;
    CreateArray(threads, sizeof(WorkerThread) * ThreadCount, true);
    for (u32 i = 0; i < ThreadCount; ++i) {
        if (!threads[i].Create()) {
            MERROR("Ошибка при создании рабочего потока. Сбой в Threadpool::Create.");
            return false;
        }
    }

    return true;
}

void Threadpool::Destroy()
{
    if (threads) {
            for (u32 i = 0; i < ThreadCount; ++i) {
                threads[i].Destroy();
            }

            DestroyArray(threads, sizeof(WorkerThread) * ThreadCount);
            threads = nullptr;
        }
        ThreadCount = 0;
}
