#pragma once

#include "memory/linear_allocator.h"

struct FrameData;

using PFN_SystemInitialize = bool(*)(u64&, void*, void*);
using PFN_SystemShutdown   = void(*)(/* void* */); 
using PFN_SystemUpdate     = bool(*)(void*, const FrameData&);

constexpr u32 M_SYSTEM_TYPE_MAX_COUNT = 512;

struct MSystem {
    u64 StateSize;
    void* state;
    PFN_SystemInitialize initialize;
    PFN_SystemShutdown shutdown;
    /// @brief Указатель функции для процедуры обновления системы, вызываемой каждый кадр. Необязательно.
    PFN_SystemUpdate update;

    enum Type : u16 {
        Memory = 0,
        Console,
        MVar,
        Event,
        Logging,
        Input,
        Platform,
        Resource,
        Shader,
        Job,
        Texture,
        Font,
        Camera,
        Renderer,
        RendererView,
        Material,
        Geometry,
        Light,
    
        // ПРИМЕЧАНИЕ: Все, что находится за пределами этого, находится в пользовательском пространстве.
        KnownMax = 255,
    
        // Максимум пользовательского пространства
        UserMax = M_SYSTEM_TYPE_MAX_COUNT,
        // Максимум, включая все типы пользовательского пространства.
        Max = UserMax
    };
};

struct ApplicationConfig;

class SystemsManager
{
    friend class Engine;
    LinearAllocator SystemsAllocator;
    MSystem systems[M_SYSTEM_TYPE_MAX_COUNT];

    static SystemsManager* state;

    /// @brief Создать линейный распределитель для всех систем (кроме памяти) для использования.
    constexpr SystemsManager() : SystemsAllocator(MEBIBYTES(64)), systems() {}
    ~SystemsManager();
public:
    SystemsManager(const SystemsManager& sm) = delete;

    static bool Initialize(SystemsManager* manager, ApplicationConfig& AppConfig);
    bool PostBootInitialize(ApplicationConfig& AppConfig);

    bool Update(const FrameData& rFrameData);

    bool Register(u16 type, PFN_SystemInitialize initialize = nullptr, PFN_SystemShutdown shutdown = nullptr, PFN_SystemUpdate update = nullptr, void* config = nullptr);

    MAPI static void* GetState(u16 type);
private:
    static bool RegisterKnownSystemsPreBoot(ApplicationConfig& AppConfig);
    bool RegisterKnownSystemsPostBoot(ApplicationConfig& AppConfig);
    void ShutdownKnownSystems();
};

