#include "job_systems.hpp"
#include "memory/linear_allocator.hpp"
#include "core/mmemory.hpp"
#include "core/mthread.hpp"
#include "core/mmutex.hpp"
#include <new>

struct JobThread {
        u8 index;
        MThread thread;
        Job::Info info;
        // Мьютекс для защиты доступа к информации этого потока.
        MMutex InfoMutex;

        // Типы задач, которые может выполнять этот поток.
        u32 TypeMask;
    };

    struct JobResultEntry {
        u16 id{};
        PFN_JobOnComplete callback{nullptr};
        u32 ParamSize{};
        void* params{nullptr};
        constexpr JobResultEntry() : id(), callback(nullptr), ParamSize(), params(nullptr) {}
        constexpr JobResultEntry(u16 id, PFN_JobOnComplete callback, u32 ParamSize, void* params)
        : id(id), callback(callback), ParamSize(ParamSize), params(params) {
            if (ParamSize > 0) {
                // Сделайте копию, так как после этого задание будет уничтожено.
                this->params = MemorySystem::Allocate(ParamSize, Memory::Job);
                MemorySystem::CopyMem(this->params, params, ParamSize);
            } else {
                this->params = nullptr;
            }
        }
    };

struct sJobSystem {
    bool running;
    u8 ThreadCount;
    JobThread JobThreads[32];

    RingQueue LowPriorityQueue;
    RingQueue NormalPriorityQueue;
    RingQueue HighPriorityQueue;

    // Мьютексы для каждой очереди, так как задание может быть запущено из другого задания (потока).
    MMutex LowPriQueueMutex;
    MMutex NormalPriQueueMutex;
    MMutex HighPriQueueMutex;

    JobResultEntry PendingResults[MAX_JOB_RESULTS];
    MMutex ResultMutex;
    // Мьютекс для массива результатов

    sJobSystem(u8 ThreadCount)
    :
    running             (true),
    ThreadCount         (ThreadCount),

    LowPriorityQueue    (sizeof(Job::Info), 1024, nullptr),
    NormalPriorityQueue (sizeof(Job::Info), 1024, nullptr),
    HighPriorityQueue   (sizeof(Job::Info), 1024, nullptr),

    LowPriQueueMutex    (),
    NormalPriQueueMutex (),
    HighPriQueueMutex   (),
    PendingResults      (),
    ResultMutex         () {}
};

static sJobSystem* pJobSystem = nullptr;

// u32 JobThreadRun(void* params);
// void StoreResult(PFN_JobOnComplete callback, u32 ParamSize, void* params);
void ProcessQueue(RingQueue& queue, MMutex& QueueMutex);

void StoreResult(PFN_JobOnComplete callback, u32 ParamSize, void* params) {
    // Создайте новую запись.
    JobResultEntry entry { INVALID::U16ID, callback, ParamSize, params };

    // Заблокируйте, найдите свободное место, сохраните, разблокируйте.
    if (!pJobSystem->ResultMutex.Lock()) {
        MERROR("Не удалось получить блокировку мьютекса для сохранения результата! Хранилище результатов может быть повреждено.");
    }
    for (u16 i = 0; i < MAX_JOB_RESULTS; ++i) {
        if (pJobSystem->PendingResults[i].id == INVALID::U16ID) {
            pJobSystem->PendingResults[i] = entry;
            pJobSystem->PendingResults[i].id = i;
            break;
        }
    }
    if (!pJobSystem->ResultMutex.Unlock()) {
        MERROR("Не удалось снять блокировку мьютекса для сохранения результата, хранилище может быть повреждено.");
    }
}

u32 JobThreadRun(void *params)
{
    u32 index = *(u32*)params;
    auto& thread = pJobSystem->JobThreads[index];
    const u64& ThreadID = thread.thread.ThreadID;
    MTRACE("Запуск потока заданий #%i (id=%#x, type=%#x).", thread.index, ThreadID, thread.TypeMask);

    // Мьютекс для блокировки информации для этого потока.
    if (!thread.InfoMutex) {
        MERROR("Не удалось создать мьютекс потока задания! Отмена потока.");
        return 0;
    }

    // Работать вечно, ожидая заданий.
    while (true) {
        if (!pJobSystem || !pJobSystem->running) {
            break;
        }

        // Блокировка и получение копии информации
        if (!thread.InfoMutex.Lock()) {
            MERROR("Не удалось получить блокировку мьютекса потока задания!");
        }
        const auto& info = thread.info;
        if (!thread.InfoMutex.Unlock()) {
            MERROR("Не удалось снять блокировку мьютекса потока задания!");
        }

        if (info.EntryPoint) {
            bool result = info.EntryPoint(info.ParamData, info.ResultData);

            // Сохраните результат для выполнения в основном потоке позже.
            // Обратите внимание, что StoreResult принимает копию ResultData, 
            // поэтому ее больше не нужно удерживать в этом потоке.
            if (result && info.OnSuccess) {
                StoreResult(info.OnSuccess, info.ResultDataSize, info.ResultData);
            } else if (!result && info.OnFail) {
                StoreResult(info.OnFail, info.ResultDataSize, info.ResultData);
            }

            // Очистите данные параметров и результаты.
            if (info.ParamData) {
                MemorySystem::Free(info.ParamData, info.ParamDataSize, Memory::Job);
            }
            if (info.ResultData) {
                MemorySystem::Free(info.ResultData, info.ResultDataSize, Memory::Job);
            }

            // Заблокируйте и сбросьте информационный объект потока
            if (!thread.InfoMutex.Lock()) {
                MERROR("Не удалось получить блокировку мьютекса потока задания!");
            }
            MemorySystem::ZeroMem(&thread.info, sizeof(Job::Info));
            if (!thread.InfoMutex.Unlock()) {
                MERROR("Не удалось снять блокировку мьютекса потока задания!");
            }
        }

        if (pJobSystem->running) {
            // ЗАДАЧА: Вероятно, следует найти лучший способ сделать это, например, 
            // заснуть, пока не поступит запрос на новое задание.
            thread.thread.Sleep(10);
        } else {
            break;
        }
    }

    // Уничтожьте мьютекс для этого потока.
    thread.InfoMutex.~MMutex();
    return 1;
}
/*
JobSystem::JobSystem(u8 ThreadCount, u32 TypeMasks[])
{
    
}
*/
bool JobSystem::Initialize(u64& MemoryRequirement, void* memory, void* config)
{
    MemoryRequirement = sizeof(sJobSystem);
    
    if (!memory) {
        return true;
    }

    auto pConfig = reinterpret_cast<JobSystemConfig*>(config);
    
    pJobSystem = new(memory) sJobSystem(pConfig->MaxJobThreadCount);

    MDEBUG("Основной идентификатор потока: %#x", GetThreadID());

    MDEBUG("Создание %i потоков заданий.", pJobSystem->ThreadCount);
    for (u8 i = 0; i < pJobSystem->ThreadCount; ++i) {
        pJobSystem->JobThreads[i].index = i;
        pJobSystem->JobThreads[i].TypeMask = pConfig->TypeMasks[i];
        auto& jThread = pJobSystem->JobThreads[i];
        if (!(jThread.thread.Create(JobThreadRun, &jThread.index, false))) {
            MFATAL("Ошибка ОС при создании потока заданий. Приложение не может продолжать работу.");
            return false;
        }
        //MMemory::ZeroMem(&jThread.info, sizeof(Job::Info));
    }

    // Аннулировать все слоты результатов
    for (u16 i = 0; i < MAX_JOB_RESULTS; ++i) {
        pJobSystem->PendingResults[i].id = INVALID::U16ID;
    }

    // Проверка создались ли мьютексы
    if (!pJobSystem->ResultMutex) {
        MERROR("Не удалось создать мьютекс результата!.");
        return false;
    }
    if (!pJobSystem->LowPriQueueMutex) {
        MERROR("Не удалось создать мьютекс очереди с низким приоритетом!.");
        return false;
    }
    if (!pJobSystem->NormalPriQueueMutex) {
        MERROR("Не удалось создать мьютекс очереди с обычным приоритетом!.");
        return false;
    }
    if (!pJobSystem->HighPriQueueMutex) {
        MERROR("Не удалось создать мьютекс очереди с высоким приоритетом!.");
        return false;
    }

    return true;
}

void JobSystem::Shutdown()
{
    if (pJobSystem) {  
        pJobSystem->running = false;
        const u64& ThreadCount = pJobSystem->ThreadCount;

        // Сначала проверьте наличие свободного потока.
        for (u8 i = 0; i < ThreadCount; ++i) {
            pJobSystem->JobThreads[i].thread.~MThread();
        }
        pJobSystem->LowPriorityQueue.~RingQueue();
        pJobSystem->NormalPriorityQueue.~RingQueue();
        pJobSystem->HighPriorityQueue.~RingQueue();

        // Уничтожить мьютексы
        pJobSystem->ResultMutex.~MMutex();
        pJobSystem->LowPriQueueMutex.~MMutex();
        pJobSystem->NormalPriQueueMutex.~MMutex();
        pJobSystem->HighPriQueueMutex.~MMutex();
        
        pJobSystem = nullptr;
    }
}

void ProcessQueue(RingQueue& queue, MMutex& QueueMutex) {
    // Сначала проверьте наличие свободной темы.
    while (queue.Length() > 0) {
        Job::Info info;
        if (!queue.Peek(&info)) {
            break;
        }

        bool ThreadFound = false;
        for (u8 i = 0; i < pJobSystem->ThreadCount; ++i) {
            auto& thread = pJobSystem->JobThreads[i];
            if ((thread.TypeMask & info.type) == 0) {
                continue;
            }

            // Проверьте, может ли поток задания обработать тип задания.
            if (!thread.InfoMutex.Lock()) {
                MERROR("Не удалось получить блокировку мьютекса потока задания!");
            }
            if (!thread.info.EntryPoint) {
                // Обязательно удалите запись из очереди.
                if (!QueueMutex.Lock()) {
                    MERROR("Не удалось получить блокировку мьютекса очереди!");
                }
                queue.Dequeue(&info);
                if (!QueueMutex.Unlock()) {
                    MERROR("Не удалось снять блокировку мьютекса очереди!");
                }
                thread.info = info;
                MTRACE("Назначение задания потоку: %u", thread.index);
                ThreadFound = true;
            }
            if (!thread.InfoMutex.Unlock()) {
                MERROR("Не удалось снять блокировку мьютекса потока задания!");
            }

            // Остановитесь после разблокировки, если был найден доступный поток.
            if (ThreadFound) {
                break;
            }
        }

        // Это означает, что все потоки в данный момент выполняют задание, 
        // поэтому дождитесь следующего обновления и повторите попытку.
        if (!ThreadFound) {
            break;
        }
    }
}

bool JobSystem::Update(void* state, const FrameData& rFrameData)
{
    if (!state || !pJobSystem->running) {
        return false;
    }

    ProcessQueue(pJobSystem->LowPriorityQueue, pJobSystem->HighPriQueueMutex);
    ProcessQueue(pJobSystem->NormalPriorityQueue, pJobSystem->NormalPriQueueMutex);
    ProcessQueue(pJobSystem->HighPriorityQueue, pJobSystem->LowPriQueueMutex);

    // Обработка ожидающих результатов.
    for (u16 i = 0; i < MAX_JOB_RESULTS; ++i) {
        // Блокировка и получение копии, разблокировка.
        if (!pJobSystem->ResultMutex.Lock()) {
            MERROR("Не удалось получить блокировку мьютекса результата!");
        }
        const auto& entry = pJobSystem->PendingResults[i];
        if (!pJobSystem->ResultMutex.Unlock()) {
            MERROR("Не удалось снять блокировку мьютекса результата!");
        }

        if (entry.id != INVALID::U16ID) {
            // Выполнение обратного вызова.
            entry.callback(entry.params);

            if (entry.params) {
                MemorySystem::Free(entry.params, entry.ParamSize, Memory::Job);
            }

            // Блокировка фактической записи, аннулирование и очистка ее
            if (!pJobSystem->ResultMutex.Lock()) {
                MERROR("Не удалось получить блокировку мьютекса результата!");
            }
            MemorySystem::ZeroMem(&pJobSystem->PendingResults[i], sizeof(JobResultEntry));
            pJobSystem->PendingResults[i].id = INVALID::U16ID;
            if (!pJobSystem->ResultMutex.Unlock()) {
                MERROR("Не удалось снять блокировку мьютекса результата!");
            }
        }
    }
    return true;
}

MAPI void JobSystem::Submit(Job::Info &info)
{
    auto* queue = &pJobSystem->NormalPriorityQueue;
    auto* QueueMutex = &pJobSystem->NormalPriQueueMutex;

    // Если задание имеет высокий приоритет, попробуйте немедленно его запустить.
    if (info.priority == Job::High) {
        queue = &pJobSystem->HighPriorityQueue;
        QueueMutex = &pJobSystem->HighPriQueueMutex;

        // Сначала проверьте наличие свободного потока, который поддерживает тип задания.
        for (u8 i = 0; i < pJobSystem->ThreadCount; ++i) {
            auto& thread = pJobSystem->JobThreads[i];
            if (thread.TypeMask & info.type) {
                bool found = false;
                if (!thread.InfoMutex.Lock()) {
                    MERROR("Не удалось получить блокировку мьютекса потока задания!");
                }
                if (!pJobSystem->JobThreads[i].info.EntryPoint) {
                    MTRACE("Задание немедленно отправлено в поток %i", pJobSystem->JobThreads[i].index);
                    pJobSystem->JobThreads[i].info = info;
                    found = true;
                }
                if (!thread.InfoMutex.Unlock()) {
                    MERROR("Не удалось снять блокировку мьютекса потока задания!");
                }
                if (found) {
                    return;
                }
            }
        }
    }

    // Если эта точка достигнута, все потоки заняты (если высокий) или он может ждать кадр.
    // Добавить в очередь и повторить попытку в следующем цикле.
    if (info.priority == Job::Low) {
        queue = &pJobSystem->LowPriorityQueue;
        QueueMutex = &pJobSystem->LowPriQueueMutex;
    }

    // ПРИМЕЧАНИЕ: Блокировка здесь на случай, если задание отправлено из другого задания/потока.
    if (!QueueMutex->Lock()) {
        MERROR("Не удалось получить блокировку на мьютексе очереди!");
    }
    queue->Enqueue(&info);
    if (!QueueMutex->Unlock()) {
        MERROR("Не удалось снять блокировку на мьютексе очереди!");
    }
    MTRACE("Задание поставлено в очередь.");
}
