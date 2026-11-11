#include "worker_thread.h"
#include "memory_system.h"

struct Work 
{
    PFN_ThreadStart WorkFn;
    void* params;
};

static u32 WorkerThreadLoop(void* params) {
    WorkerThread* thread = (WorkerThread*)params;

    while (true) {
        thread->QueueMutex.Lock();
        if (thread->WorkQueue.ElementCount() == 0) {
            thread->QueueMutex.Unlock();
            break;
        }
        Work w;
        if (!thread->WorkQueue.Pop(&w)) {
            MERROR("Не удалось извлечь задачу из очереди задач.");
            thread->QueueMutex.Unlock();
            return 0;
        }
        thread->QueueMutex.Unlock();
        w.WorkFn(w.params);
    }

    MTRACE("Работа рабочего потока завершена.");

    return 1;
}

constexpr WorkerThread::WorkerThread() : thread(), WorkQueue(sizeof(Work)), QueueMutex() {}

bool WorkerThread::Create()
{
    if (!WorkQueue.Create(sizeof(Work))) {
        MERROR("Не удалось создать внутреннюю очередь задач для рабочего потока.");
        return false;
    }

    if (!QueueMutex.Create()) {
        MERROR("Не удалось создать мьютекс для внутренней очереди задач рабочего потока.");
        return false;
    }

    return true;
}

void WorkerThread::Destroy()
{
    WorkQueue.Destroy();
    QueueMutex.Destroy();
    thread.Destroy();
    MemorySystem::ZeroMem(this, sizeof(WorkerThread));
}

bool WorkerThread::Add(PFN_ThreadStart WorkFn, void *params)
{
    if (!WorkFn) {
        MERROR("Для вызова WorkerThread::Add требуются корректные указатели на объект worker_thread и на функцию, выполняющую работу.");
        return false;
    }

    QueueMutex.Lock();
    Work w {
        .WorkFn = WorkFn,
        .params = params
    }; 

    if (!WorkQueue.Push(&w)) {
        MERROR("Не удалось поместить задачу в очередь.");
        QueueMutex.Unlock();
        return false;
    }
    QueueMutex.Unlock();

    return true;
}

bool WorkerThread::Start()
{
    // При необходимости создайте внутренний поток.
    if (!thread.Create(WorkerThreadLoop, this, false)) {
        MERROR("Не удалось создать внутренний поток рабочего потока.");
        return false;
    }

    return true;
}

bool WorkerThread::Wait()
{
    return thread.Wait();
}
