#pragma once

#include "containers/ring_queue.hpp"
#include "core/mmutex.hpp"
#include "core/mthread.hpp"
#include "core/memory_system.h"

constexpr i32 MAX_JOB_RESULTS = 512; // Максимальное количество результатов задания, которые можно сохранить одновременно.

/// @brief Определение указателя функции для заданий.
typedef bool (*PFN_JobStart)(void*, void*);

/// @brief Определение указателя функции для завершения задания.
typedef void (*PFN_JobOnComplete)(void*);

struct FrameData;

namespace Job {
    /// @brief Описывает тип задания.
    enum Type {
        General      = 0x02,    // Общая работа, не имеющая особых требований к потоку. Это означает, что не имеет значения, в каком потоке выполняется эта работа.
        ResourceLoad = 0x04,    // Задание по загрузке ресурсов. Ресурсы всегда должны загружаться в одном потоке, чтобы избежать потенциального переполнения диска.
        GPUResource  = 0x08,    // Задания, использующие ресурсы GPU, должны быть привязаны к потоку, использующему этот тип задания. Многопоточные рендереры будут использовать определенный поток задания, и этот тип задания будет выполняться в этом потоке. Для однопоточных рендереров это будет в основном потоке.
    };

    /// @brief Определяет, какую очередь заданий использует задание. Очередь с высоким приоритетом всегда исчерпывается первой перед обработкой очереди с нормальным приоритетом, которая также должна быть исчерпана перед обработкой очереди с низким приоритетом.
    enum Priority {
        Low,    // Задание с самым низким приоритетом, используемое для задач, которые могут подождать выполнения, если возникнет такая необходимость, например, очистка журнала.
        Normal, // Задание с обычным приоритетом. Следует использовать для задач со средним приоритетом, например, загрузка активов.
        High    // Задание с самым высоким приоритетом. Следует использовать экономно и только для критических по времени операций.
    };

    /// @brief Описывает задание, которое должно быть запущено.
    struct Info {
        Type                    type;   // Тип задания. Используется для определения того, в каком потоке выполняется задание.
        Priority            priority;   // Приоритет этого задания. Задания с более высоким приоритетом, очевидно, выполняются раньше.
        PFN_JobStart      EntryPoint;   // Указатель функции, который должен быть вызван при запуске задания. Обязательно.
        PFN_JobOnComplete  OnSuccess;   // Указатель функции, который должен быть вызван при успешном завершении задания. Необязательно.
        PFN_JobOnComplete     OnFail;   // Указатель функции, который должен быть вызван при успешном сбое задания. Необязательно.
        void*              ParamData;   // Данные, которые должны быть переданы в точку входа при выполнении.
        u32            ParamDataSize;   // Размер данных, передаваемых заданию.
        void*             ResultData;   // Данные, которые должны быть переданы в функцию успеха/неудачи при выполнении, если они существуют.
        u32           ResultDataSize;   // Размер данных, передаваемых в функцию успеха/неудачи.
        constexpr Info() 
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
        constexpr Info(PFN_JobStart EntryPoint, PFN_JobOnComplete OnSuccess, PFN_JobOnComplete OnFail, void* ParamData, u32 ParamDataSize, u32 ResultDataSize, Type type = Job::General, Priority priority = Job::Normal)
        : type(type), priority(priority), 
        EntryPoint(EntryPoint), OnSuccess(OnSuccess), OnFail(OnFail), 
        ParamData(), ParamDataSize(ParamDataSize), 
        ResultData(ResultDataSize > 0 ? MemorySystem::Allocate(ResultDataSize, Memory::Job) : nullptr), 
        ResultDataSize(ResultDataSize) {
            if (ParamDataSize) {
                this->ParamData = MemorySystem::Allocate(ParamDataSize, Memory::Job);
                MemorySystem::CopyMem(this->ParamData, ParamData, ParamDataSize);
            } else {
                this->ParamData = nullptr;
            }
        }
    };
}

struct JobSystemConfig {
    /// @brief Максимальное количество потоков заданий, которые необходимо раскрутить. 
    /// Не должно превышать количество ядер ЦП, минус один для учета основного потока.
    u8 MaxJobThreadCount;
    /// @brief Коллекция масок типов для каждого потока работы. Должна соответствовать MaxJobThreadCount.
    u32* TypeMasks;
};

namespace JobSystem
{
    bool Initialize(u64& MemoryRequirement, void* memory, void* config);
    void Shutdown();

    // JobSystem* Instance() { return state; }

    /// @brief Обновляет систему заданий. Должно происходить один раз в цикл обновления.
    bool Update(void* state, const FrameData& rFrameData);
    /// @brief Отправляет предоставленное задание в очередь на выполнение.
    /// @param info Описание задания, которое должно быть выполнено.
    MAPI void Submit(Job::Info& info);
};
