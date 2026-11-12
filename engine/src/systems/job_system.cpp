#include "job_system.h"
#include "memory/linear_allocator.h"
#include "core/asserts.h"
#include "core/memory_system.h"
#include "core/mthread.h"
#include "core/mmutex.h"

#include <new>

JobSystem::JobSystem(Config& config)
:
running             (true),
ThreadCount         (config.MaxJobThreadCount),

LowPriorityQueue    (sizeof(JobInfo), 1024, nullptr),
NormalPriorityQueue (sizeof(JobInfo), 1024, nullptr),
HighPriorityQueue   (sizeof(JobInfo), 1024, nullptr),

LowPriQueueMutex    (),
NormalPriQueueMutex (),
HighPriQueueMutex   (),
PendingResults      (),
ResultMutex         () 
{
    MDEBUG("Основной идентификатор потока: %#x", GetThreadID());

    MDEBUG("Создание %i потоков заданий.", ThreadCount);
    for (u8 i = 0; i < ThreadCount; ++i) {
        JobThreads[i].index = i;
        JobThreads[i].TypeMask = config.TypeMasks[i];
        auto& jThread = JobThreads[i];
        if (!(jThread.thread.Create(JobThreadRun, &jThread.index, false))) {
            MFATAL("Ошибка ОС при создании потока заданий. Приложение не может продолжать работу.");
            return;
        }
        //MMemory::ZeroMem(&jThread.info, sizeof(Job::Info));
    }

    // Аннулировать все слоты результатов
    // for (u16 i = 0; i < MAX_JOB_RESULTS; ++i) {
    //     PendingResults[i].id = INVALID::U16ID;
    // }

    // Проверка создались ли мьютексы
    if (!ResultMutex) {
        MERROR("Не удалось создать мьютекс результата!.");
        return;
    }
    if (!LowPriQueueMutex) {
        MERROR("Не удалось создать мьютекс очереди с низким приоритетом!.");
        return;
    }
    if (!NormalPriQueueMutex) {
        MERROR("Не удалось создать мьютекс очереди с обычным приоритетом!.");
        return;
    }
    if (!HighPriQueueMutex) {
        MERROR("Не удалось создать мьютекс очереди с высоким приоритетом!.");
        return;
    }
}

void JobSystem::StoreResult(PFN_JobOnComplete callback, u32 ParamSize, void* params) {
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

u32 JobSystem::JobThreadRun(void *params)
{
    u32 index = *(u32*)params;
    auto& thread = pJobSystem->JobThreads[index];
    const u32 ThreadID = thread.thread.ThreadID;
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
        JobInfo& info = thread.info;
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
                info.ParamData = nullptr;
            }
            if (info.ResultData) {
                MemorySystem::Free(info.ResultData, info.ResultDataSize, Memory::Job);
                info.ResultData = nullptr;
            }

            // Заблокируйте и сбросьте информационный объект потока
            if (!thread.InfoMutex.Lock()) {
                MERROR("Не удалось получить блокировку мьютекса потока задания!");
            }
            MemorySystem::ZeroMem(&thread.info, sizeof(JobInfo));
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

bool JobSystem::Initialize(void *ptr, Config JobSysConfig)
{
    if (!ptr) { return false; }
    
    pJobSystem = (JobSystem*)ptr;
    new(ptr) JobSystem(JobSysConfig);

    return true;
}

void JobSystem::Shutdown()
{
    running = false;

    // Сначала проверьте наличие свободного потока.
    for (u8 i = 0; i < ThreadCount; ++i) {
        JobThreads[i].thread.~MThread();
    }
    LowPriorityQueue.~RingQueue();
    NormalPriorityQueue.~RingQueue();
    HighPriorityQueue.~RingQueue();

    // Уничтожить мьютексы
    ResultMutex.~MMutex();
    LowPriQueueMutex.~MMutex();
    NormalPriQueueMutex.~MMutex();
    HighPriQueueMutex.~MMutex();
        
    pJobSystem = nullptr;
}

void JobSystem::ProcessQueue(RingQueue& queue, MMutex& QueueMutex) {
    // Сначала проверьте наличие свободной темы.
    while (queue.Length() > 0) {
        JobInfo info;
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

bool JobSystem::Update(void* state, FrameData& rFrameData)
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

MAPI void JobSystem::Submit(JobInfo &info)
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

JobInfo JobSystem::CreateJobInfo(PFN_JobStart EntryPoint, PFN_JobOnComplete OnSuccess, PFN_JobOnComplete OnFail, void *ParamData, u32 ParamDataSize, u32 ResultDataSize, Job::Type type, Job::Priority priority, u8 DependencyCount, u16 *dependencies)
{
    JobInfo job {
        .type = type,
        .priority = priority,
        .EntryPoint = EntryPoint,
        .OnSuccess = OnSuccess,
        .OnFail = OnFail,
        .ParamDataSize = ParamDataSize,
        .ResultDataSize = ResultDataSize
    };

    // Технически задачи могут создаваться в процессе выполнения других задач (например, в другом потоке), 
    // поэтому обязательно используйте блокировку при внесении таких изменений.
    if (!pJobSystem->JobStatusMutex.Lock()) {
        MERROR("Не удалось заблокировать мьютекс состояния задания!");
    }

    // ЗАДАЧА: Pack booleans.
    MASSERT_MSG(pJobSystem->CurrentJobID < INVALID::U16ID, "Переполнение идентификатора в системе заданий — необходимо упаковать логические значения.");
    job.id = pJobSystem->CurrentJobID;
    pJobSystem->CurrentJobID++;
    pJobSystem->JobStatuses[job.id] = false;
    if (!pJobSystem->JobStatusMutex.Unlock()) {
        MERROR("Не удалось разблокировать мьютекс состояния задания!");
    }

    if (ParamDataSize) {
        job.ParamData = MemorySystem::Allocate(ParamDataSize, Memory::Job);
        MemorySystem::CopyMem(job.ParamData, ParamData, ParamDataSize);
    } else {
        job.ParamData = nullptr;
    }

    if (ResultDataSize) {
        job.ResultData = MemorySystem::Allocate(ResultDataSize, Memory::Job, true);
    } else {
        job.ResultData = nullptr;
    }

    job.DependencyCount = DependencyCount;
    if (DependencyCount) {
        CreateArray(job.DependencyIds, sizeof(u16) * DependencyCount);
        MemorySystem::CopyMem(job.DependencyIds, dependencies, sizeof(u16) * DependencyCount);
    } else {
        job.DependencyIds = nullptr;
    }

    return job;
}

bool JobSystem::QueryJobComplete()
{
    bool status = INVALID::ID;
    if (!pJobSystem->JobStatusMutex.Lock()) {
        MERROR("Не удалось заблокировать мьютекс состояния задания!");
    }
    status = pJobSystem->JobStatuses[job_id];
    if (!pJobSystem->JobStatusMutex.Unlock()) {
        MERROR("Не удалось разблокировать мьютекс состояния задания!");
    }
    return status;
}
