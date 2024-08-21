#pragma once

#include "containers/ring_queue.hpp"
#include "core/mmutex.hpp"
#include "core/mthread.hpp"
#include "core/mmemory.hpp"

constexpr i32 MAX_JOB_RESULTS = 512; // Максимальное количество результатов задания, которые можно сохранить одновременно.

/// @brief Определение указателя функции для заданий.
typedef bool (*PFN_JobStart)(void*, void*);

/// @brief Определение указателя функции для завершения задания.
typedef void (*PFN_JobOnComplete)(void*);

/// @brief Описывает тип задания.
namespace JobType {
    enum E {
        General      = 0x02,    // Общая работа, не имеющая особых требований к потоку. Это означает, что не имеет значения, в каком потоке выполняется эта работа.
        ResourceLoad = 0x04,    // Задание по загрузке ресурсов. Ресурсы всегда должны загружаться в одном потоке, чтобы избежать потенциального переполнения диска.
        GPUResource  = 0x08,    // Задания, использующие ресурсы GPU, должны быть привязаны к потоку, использующему этот тип задания. Многопоточные рендереры будут использовать определенный поток задания, и этот тип задания будет выполняться в этом потоке. Для однопоточных рендереров это будет в основном потоке.
    };
}

/// @brief Определяет, какую очередь заданий использует задание. Очередь с высоким приоритетом всегда исчерпывается первой перед обработкой очереди с нормальным приоритетом, которая также должна быть исчерпана перед обработкой очереди с низким приоритетом.
namespace JobPriority {
    enum E {
        Low,    // Задание с самым низким приоритетом, используемое для задач, которые могут подождать выполнения, если возникнет такая необходимость, например, очистка журнала.
        Normal, // Задание с обычным приоритетом. Следует использовать для задач со средним приоритетом, например, загрузка активов.
        High    // Задание с самым высоким приоритетом. Следует использовать экономно и только для критических по времени операций.
    };
}

/// @brief Описывает задание, которое должно быть запущено.
struct JobInfo {
    JobType::E              type;   // Тип задания. Используется для определения того, в каком потоке выполняется задание.
    JobPriority::E      priority;   // Приоритет этого задания. Задания с более высоким приоритетом, очевидно, выполняются раньше.
    PFN_JobStart      EntryPoint;   // Указатель функции, который должен быть вызван при запуске задания. Обязательно.
    PFN_JobOnComplete  OnSuccess;   // Указатель функции, который должен быть вызван при успешном завершении задания. Необязательно.
    PFN_JobOnComplete     OnFail;   // Указатель функции, который должен быть вызван при успешном сбое задания. Необязательно.
    void*              ParamData;   // Данные, которые должны быть переданы в точку входа при выполнении.
    u32            ParamDataSize;   // Размер данных, передаваемых заданию.
    void*             ResultData;   // Данные, которые должны быть переданы в функцию успеха/неудачи при выполнении, если они существуют.
    u32           ResultDataSize;   // Размер данных, передаваемых в функцию успеха/неудачи.
    constexpr JobInfo() 
    : type(), priority(), EntryPoint(), OnSuccess(), OnFail(), ParamData(), ParamDataSize(), ResultData(), ResultDataSize() {}
    /// @brief Создает новое задание. По умолчанию тиип (General), а приоритет (Normal).
    /// @param EntryPoint указатель на функцию, которая будет вызвана при запуске задания. Обязательно.
    /// @param OnSuccess указатель на функцию, которая будет вызвана при успешном завершении задания. Необязательно.
    /// @param OnFail указатель на функцию, которая будет вызвана при сбое задания. Необязательно.
    /// @param ParamData данные, которые будут переданы в точку входа при выполнении.
    /// @param ParamDataSize данные, которые будут переданы в обратный вызов entry_point. Передайте 0, если не используются.
    /// @param ResultDataSize размер данных результата, которые будут переданы в обратный вызов success. Передайте 0, если не используются.
    /// @param type тип задания. Используется для определения того, в каком потоке выполняется задание.
    /// @param priority приоритет этого задания. Задания с более высоким приоритетом, очевидно, выполняются раньше.
    constexpr JobInfo(PFN_JobStart EntryPoint, PFN_JobOnComplete OnSuccess, PFN_JobOnComplete OnFail, void* ParamData, u32 ParamDataSize, u32 ResultDataSize, JobType::E type = JobType::General, JobPriority::E priority = JobPriority::Normal)
    : type(type), priority(priority), 
    EntryPoint(EntryPoint), OnSuccess(OnSuccess), OnFail(OnFail), 
    ParamData(), ParamDataSize(ParamDataSize), 
    ResultData(ResultDataSize > 0 ? MMemory::Allocate(ResultDataSize, Memory::Job) : nullptr), 
    ResultDataSize(ResultDataSize) {
        if (ParamDataSize) {
            this->ParamData = MMemory::Allocate(ParamDataSize, Memory::Job);
            MMemory::CopyMem(this->ParamData, ParamData, ParamDataSize);
        } else {
            this->ParamData = nullptr;
        }
    }
};

class JobSystem
{
    struct JobThread {
        u8 index;
        MThread thread;
        JobInfo info;
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
                this->params = MMemory::Allocate(ParamSize, Memory::Job);
                MMemory::CopyMem(this->params, params, ParamSize);
            } else {
                this->params = nullptr;
            }
        }
    };
private:
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

    static JobSystem* state;

    JobSystem(u8 ThreadCount, u32 TypeMasks[]);
    ~JobSystem();
public:
    JobSystem(const JobSystem&) = delete;
    JobSystem& operator= (const JobSystem&) = delete;

    static bool Initialize(u8 MaxJobThreadCount, u32 TypeMasks[]);
    static void Shutdown();

    static JobSystem* Instance() { return state; }

    /// @brief Обновляет систему заданий. Должно происходить один раз в цикл обновления.
    void Update();
    /// @brief Отправляет предоставленное задание в очередь на выполнение.
    /// @param info Описание задания, которое должно быть выполнено.
    MAPI void Submit(JobInfo& info);
private:
    static u32 JobThreadRun(void* params);
    void StoreResult(PFN_JobOnComplete callback, u32 ParamSize, void* params);
    void ProcessQueue(RingQueue& queue, MMutex& QueueMutex);
};
