#pragma once

#include "defines.hpp"
#include "memory/linear_allocator.hpp"

using PFN_SystemInitialize = bool(*)(u64&, void*, void*);
using PFN_SystemShutdown   = void(*)(/* void* */); 
using PFN_SystemUpdate     = bool(*)(void*, f32);

constexpr u32 K_SYSTEM_TYPE_MAX_COUNT = 512;

struct MSystem {
    u64 StateSize;
    void* state;
    PFN_SystemInitialize initialize;
    PFN_SystemShutdown shutdown;
    /// @brief Указатель функции для процедуры обновления системы, вызываемой каждый кадр. Необязательно.
    PFN_SystemUpdate update;

    enum Type {
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
    
        // ПРИМЕЧАНИЕ: Все, что находится за пределами этого, находится в пользовательском пространстве.
        KnownMax = 255,
    
        // Максимум пользовательского пространства
        UserMax = K_SYSTEM_TYPE_MAX_COUNT,
        // Максимум, включая все типы пользовательского пространства.
        Max = UserMax
    };
};

struct ApplicationConfig;

class SystemsManager
{
public:
    LinearAllocator SystemsAllocator;
    MSystem systems[K_SYSTEM_TYPE_MAX_COUNT];

    /// @brief Создать линейный распределитель для всех систем (кроме памяти) для использования.
    constexpr SystemsManager() : SystemsAllocator(MEBIBYTES(64)), systems() {}
    ~SystemsManager();

    bool Initialize(ApplicationConfig& AppConfig);
    bool PostBootInitialize(ApplicationConfig& AppConfig);

    bool Update(u32 DeltaTime);

    bool Register(u16 type, PFN_SystemInitialize initialize = nullptr, PFN_SystemShutdown shutdown = nullptr, PFN_SystemUpdate update = nullptr, void* config = nullptr);
private:
    bool RegisterKnownSystemsPreBoot(ApplicationConfig& AppConfig);
    bool RegisterKnownSystemsPostBoot(ApplicationConfig& AppConfig);
    void ShutdownKnownSystems();
};

