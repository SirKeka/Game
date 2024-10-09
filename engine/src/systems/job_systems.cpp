#include "job_systems.hpp"
#include "memory/linear_allocator.hpp"
#include "core/mmemory.hpp"
#include "core/mthread.hpp"
#include "core/mmutex.hpp"
#include <new>

JobSystem *JobSystem::state = nullptr;

void JobSystem::StoreResult(PFN_JobOnComplete callback, u32 ParamSize, void* params) {
    // Создайте новую запись.
    JobResultEntry entry { INVALID::U16ID, callback, ParamSize, params };

    // Заблокируйте, найдите свободное место, сохраните, разблокируйте.
    if (!ResultMutex.Lock()) {
        MERROR("Не удалось получить блокировку мьютекса для сохранения результата! Хранилище результатов может быть повреждено.");
    }
    for (u16 i = 0; i < MAX_JOB_RESULTS; ++i) {
        if (PendingResults[i].id == INVALID::U16ID) {
            PendingResults[i] = entry;
            PendingResults[i].id = i;
            break;
        }
    }
    if (!ResultMutex.Unlock()) {
        MERROR("Не удалось снять блокировку мьютекса для сохранения результата, хранилище может быть повреждено.");
    }
}

u32 JobSystem::JobThreadRun(void *params)
{
    u32 index = *(u32*)params;
    auto& thread = state->JobThreads[index];
    const u64& ThreadID = thread.thread.ThreadID;
    MTRACE("Запуск потока заданий #%i (id=%#x, type=%#x).", thread.index, ThreadID, thread.TypeMask);

    // Мьютекс для блокировки информации для этого потока.
    if (!thread.InfoMutex) {
        MERROR("Не удалось создать мьютекс потока задания! Отмена потока.");
        return 0;
    }

    // Работать вечно, ожидая заданий.
    while (true) {
        if (!state || !state->running) {
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
                state->StoreResult(info.OnSuccess, info.ResultDataSize, info.ResultData);
            } else if (!result && info.OnFail) {
                state->StoreResult(info.OnFail, info.ResultDataSize, info.ResultData);
            }

            // Очистите данные параметров и результаты.
            if (info.ParamData) {
                MMemory::Free(info.ParamData, info.ParamDataSize, Memory::Job);
            }
            if (info.ResultData) {
                MMemory::Free(info.ResultData, info.ResultDataSize, Memory::Job);
            }

            // Заблокируйте и сбросьте информационный объект потока
            if (!thread.InfoMutex.Lock()) {
                MERROR("Не удалось получить блокировку мьютекса потока задания!");
            }
            MMemory::ZeroMem(&thread.info, sizeof(JobInfo));
            if (!thread.InfoMutex.Unlock()) {
                MERROR("Не удалось снять блокировку мьютекса потока задания!");
            }
        }

        if (state->running) {
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

JobSystem::JobSystem(u8 ThreadCount, u32 TypeMasks[])
: 
    running             (true),
    ThreadCount         (ThreadCount),

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
        JobThreads[i].TypeMask = TypeMasks[i];
        auto& jThread = JobThreads[i];
        if (!(jThread.thread.Create(JobThreadRun, &jThread.index, false))) {
            MFATAL("Ошибка ОС при создании потока заданий. Приложение не может продолжать работу.");
            return;
        }
        //MMemory::ZeroMem(&jThread.info, sizeof(JobInfo));
    }
}

JobSystem::~JobSystem()
{
    if(running){   
        running = false;
        /*const u64& ThreadCount = ThreadCount;

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
        HighPriQueueMutex.~MMutex();*/
    }
}

bool JobSystem::Initialize(u8 MaxJobThreadCount, u32 TypeMasks[], LinearAllocator& SystemAllocator)
{
    state = reinterpret_cast<JobSystem*>(SystemAllocator.Allocate(sizeof(JobSystem)));
    new(state) JobSystem(MaxJobThreadCount, TypeMasks);

    // Аннулировать все слоты результатов
    for (u16 i = 0; i < MAX_JOB_RESULTS; ++i) {
        state->PendingResults[i].id = INVALID::U16ID;
    }

    // Проверка создались ли мьютексы
    if (!state->ResultMutex) {
        MERROR("Не удалось создать мьютекс результата!.");
        return false;
    }
    if (!state->LowPriQueueMutex) {
        MERROR("Не удалось создать мьютекс очереди с низким приоритетом!.");
        return false;
    }
    if (!state->NormalPriQueueMutex) {
        MERROR("Не удалось создать мьютекс очереди с обычным приоритетом!.");
        return false;
    }
    if (!state->HighPriQueueMutex) {
        MERROR("Не удалось создать мьютекс очереди с высоким приоритетом!.");
        return false;
    }

    return true;
}

void JobSystem::Shutdown()
{
    if (state) {
        state = nullptr;
    }
}

void JobSystem::ProcessQueue(RingQueue& queue, MMutex& QueueMutex) {
    // Сначала проверьте наличие свободной темы.
    while (queue.Length() > 0) {
        JobInfo info;
        if (!queue.Peek(&info)) {
            break;
        }

        bool ThreadFound = false;
        for (u8 i = 0; i < ThreadCount; ++i) {
            auto& thread = JobThreads[i];
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

void JobSystem::Update()
{
    if (!state || !running) {
        return;
    }

    ProcessQueue(LowPriorityQueue, HighPriQueueMutex);
    ProcessQueue(NormalPriorityQueue, NormalPriQueueMutex);
    ProcessQueue(HighPriorityQueue, LowPriQueueMutex);

    // Обработка ожидающих результатов.
    for (u16 i = 0; i < MAX_JOB_RESULTS; ++i) {
        // Блокировка и получение копии, разблокировка.
        if (!ResultMutex.Lock()) {
            MERROR("Не удалось получить блокировку мьютекса результата!");
        }
        const auto& entry = PendingResults[i];
        if (!ResultMutex.Unlock()) {
            MERROR("Не удалось снять блокировку мьютекса результата!");
        }

        if (entry.id != INVALID::U16ID) {
            // Выполнение обратного вызова.
            entry.callback(entry.params);

            if (entry.params) {
                MMemory::Free(entry.params, entry.ParamSize, Memory::Job);
            }

            // Блокировка фактической записи, аннулирование и очистка ее
            if (!ResultMutex.Lock()) {
                MERROR("Не удалось получить блокировку мьютекса результата!");
            }
            MMemory::ZeroMem(&PendingResults[i], sizeof(JobResultEntry));
            PendingResults[i].id = INVALID::U16ID;
            if (!ResultMutex.Unlock()) {
                MERROR("Не удалось снять блокировку мьютекса результата!");
            }
        }
    }
}

MAPI void JobSystem::Submit(JobInfo &info)
{
    auto* queue = &NormalPriorityQueue;
    auto* QueueMutex = &NormalPriQueueMutex;

    // Если задание имеет высокий приоритет, попробуйте немедленно его запустить.
    if (info.priority == JobPriority::High) {
        queue = &HighPriorityQueue;
        QueueMutex = &HighPriQueueMutex;

        // Сначала проверьте наличие свободного потока, который поддерживает тип задания.
        for (u8 i = 0; i < ThreadCount; ++i) {
            auto& thread = JobThreads[i];
            if (thread.TypeMask & info.type) {
                bool found = false;
                if (!thread.InfoMutex.Lock()) {
                    MERROR("Не удалось получить блокировку мьютекса потока задания!");
                }
                if (!JobThreads[i].info.EntryPoint) {
                    MTRACE("Задание немедленно отправлено в поток %i", JobThreads[i].index);
                    JobThreads[i].info = info;
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
    if (info.priority == JobPriority::Low) {
        queue = &LowPriorityQueue;
        QueueMutex = &LowPriQueueMutex;
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
