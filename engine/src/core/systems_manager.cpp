#include "systems_manager.h"

#include "core/console.h"
#include "core/engine.h"
#include "core/event.h"
#include "core/memory_system.h"
#include "core/mvar.h"
#include "core/input.h"
#include "platform/platform.hpp"

#include "renderer/rendering_system.h"

#include "systems/camera_system.hpp"
#include "systems/geometry_system.h"
#include "systems/job_systems.h"
#include "systems/light_system.h"
#include "systems/material_system.h"
#include "systems/resource_system.h"
#include "systems/shader_system.h"
#include "systems/texture_system.h"

SystemsManager::~SystemsManager()
{
    ShutdownKnownSystems();
}

bool SystemsManager::Initialize(SystemsManager* manager, ApplicationConfig &AppConfig)
{
    if (!state) {
        state = manager;
        // Зарегистрировать известные системы
        return RegisterKnownSystemsPreBoot(AppConfig);
    }
    return false;
}

bool SystemsManager::PostBootInitialize(ApplicationConfig &AppConfig)
{
    return RegisterKnownSystemsPostBoot(AppConfig);
}

bool SystemsManager::Update(FrameData& rFrameData)
{
    for (u32 i = 0; i < M_SYSTEM_TYPE_MAX_COUNT; ++i) {
        MSystem& system = systems[i];
        if (system.Update) {
            if (!system.Update(system.state, rFrameData)) {
                MERROR("Ошибка обновления системы для типа: %i", i);
            }
        }
    }
    return true;
}

void SystemsManager::RendererFramePrepare(const FrameData &rFrameData)
{
    for (u32 i = 0; i < M_SYSTEM_TYPE_MAX_COUNT; ++i) {
        MSystem& system = systems[i];
        if (system.RenderPrepareFrame) {
            system.RenderPrepareFrame(system.state, rFrameData);
        }
    }
}

bool SystemsManager::Register(u16 type, void *system, PFN_SystemUpdate update, PFN_SystemShutdown shutdown, PFN_SystemRenderPrepareFrame RenderPrepareFrame)
{   
    // MSystem sys;
    // sys.Initialize = initialize;
    // sys.Shutdown = shutdown;
    // sys.Update = update;

    // Вызов инициализации, выделение памяти, повторный вызов инициализации с выделенным блоком.
    // if (sys.initialize) {
    //     if (!sys.Initialize(size, nullptr, config)) {
    //         MERROR("Не удалось зарегистрировать систему — инициализация не удалась.");
    //         return false;
    //     }

    //     sys.state = SystemsAllocator.Allocate(size);

    //     if (!sys.Initialize(size, sys.state, config)) {
    //         MERROR("Не удалось зарегистрировать систему — инициализация не удалась.");
    //         return false;
    //     }
    // } else {
    //     if (type != MSystem::Memory) {
    //         MERROR("Инициализация требуется для типов, кроме MSystem::Memory.");
    //         return false;
    //     }
    // }

    systems[type].state = system;
    systems[type].Update = update;
    systems[type].Shutdown = shutdown;
    systems[type].RenderPrepareFrame = RenderPrepareFrame;

    return true;
}

void* SystemsManager::GetState(u16 type)
{
    if (!state) { return nullptr; }
    return state->systems[type].state;
}

bool SystemsManager::RegisterKnownSystemsPreBoot(ApplicationConfig &AppConfig)
{
    void* ptr = nullptr;
    u64 MemoryRequirement = 0;

    #pragma region Память
    if (!state->Register(Memory, MemorySystem::GetInstance())) {
        MERROR("Не удалось зарегистрировать систему памяти.");
        return false;
    }
    #pragma endregion

    #pragma region Консоль
    Console::Initialize(MemoryRequirement, ptr = nullptr);
    ptr = state->SystemsAllocator.Allocate(MemoryRequirement);
    if (!state->Register(Console, Console::Initialize(MemoryRequirement, ptr))) {
        MERROR("Не удалось зарегистрировать консольную систему.");
        return false;
    }
    #pragma endregion

    #pragma region MVars
    MVar::Initialize(MemoryRequirement, ptr = nullptr);
    ptr = state->SystemsAllocator.Allocate(MemoryRequirement);
    if (!state->Register(MVar, MVar::Initialize(MemoryRequirement, ptr))) {
        MERROR("Не удалось зарегистрировать систему KVar.");
        return false;
    }
    #pragma endregion

    #pragma region События
    EventSystem::Initialize(MemoryRequirement, ptr = nullptr);
    ptr = state->SystemsAllocator.Allocate(MemoryRequirement);
    if (!state->Register(Event, EventSystem::Initialize(MemoryRequirement, ptr))) {
        MERROR("Не удалось зарегистрировать систему событий.");
        return false;
    }
    #pragma endregion

    #pragma region Журнал
    Log::Initialize(MemoryRequirement, ptr = nullptr);
    ptr = state->SystemsAllocator.Allocate(MemoryRequirement);
    if (!state->Register(Logging, Log::Initialize(MemoryRequirement, ptr))) {
        MERROR("Не удалось зарегистрировать систему логирования.");
        return false;
    }
    #pragma endregion

    #pragma region Ввод
    InputSystem::Initialize(MemoryRequirement, ptr = nullptr);
    ptr = state->SystemsAllocator.Allocate(MemoryRequirement);
    if (!state->Register(Input, InputSystem::Initialize(MemoryRequirement, ptr))) {
        MERROR("Не удалось зарегистрировать систему ввода.");
        return false;
    }
    #pragma endregion

    #pragma region Оконная система
    WindowSystem::Initialize(MemoryRequirement, ptr = nullptr, AppConfig);
    ptr = state->SystemsAllocator.Allocate(MemoryRequirement);
    if (!state->Register(Platform, WindowSystem::Initialize(MemoryRequirement, ptr, AppConfig))) {
        MERROR("Не удалось зарегистрировать систему окон.");
        return false;
    }
    #pragma endregion

    #pragma region Система ресурсов.
    ResourceSystemConfig ResourceSysConfig { 32, "../assets" };  // ЗАДАЧА: Вероятно, приложение должно это настроить.
    ResourceSystem::Initialize(MemoryRequirement, ptr = nullptr, ResourceSysConfig);
    ptr = state->SystemsAllocator.Allocate(MemoryRequirement);
    if (!state->Register(Resource, ResourceSystem::Initialize(MemoryRequirement, ptr, ResourceSysConfig))) {
        MERROR("Не удалось зарегистрировать систему ресурсов.");
        return false;
    }
    #pragma endregion

    #pragma region Система шейдеров
    ShaderSystem::Config ShaderSysConfig{ 1024, 128, 31, 31 };
    ShaderSystem::Initialize(MemoryRequirement, ptr = nullptr, ShaderSysConfig);
    ptr = state->SystemsAllocator.Allocate(MemoryRequirement);
    if (!state->Register(Shader, ShaderSystem::Initialize(MemoryRequirement, ptr, ShaderSysConfig))) {
        MERROR("Не удалось зарегистрировать систему шейдеров.");
        return false;
    }
    #pragma endregion

    #pragma region Система отрисовки
    u64 PluginSize = 0;
    AppConfig.PluginCreate(nullptr, &PluginSize);
    RenderingSystem::Config RendererSysConfig{ AppConfig.name, AppConfig.PluginCreate };
    ptr = state->SystemsAllocator.Allocate(sizeof(RenderingSystem) + PluginSize);
    if (!state->Register(Renderer, new(ptr) RenderingSystem(RendererSysConfig))) {
        MERROR("Не удалось зарегистрировать систему отрисовки.");
        return false;
    }
    #pragma endregion

    #pragma region Потоки
    bool RendererMultithreaded = GetRenderingSystem()->IsMultithreaded();

    // Это действительно количество ядер. Вычтите 1, чтобы учесть, что основной поток уже используется.
    u8 ThreadCount = PlatformGetProcessorCount() - 1;
    if (ThreadCount < 1) {
        MFATAL("Ошибка: платформа сообщила количество процессоров (минус один для основного потока) как %i. Нужен как минимум один дополнительный поток для системы заданий.", ThreadCount);
        return false;
    } else {
        MTRACE("Доступные потоки: %i", ThreadCount);
    }

    // Ограничьте количество потоков.
    const u8 MaxThreadCount = 15;
    if (ThreadCount > MaxThreadCount) {
        MTRACE("Доступные потоки в системе — %i, но будут ограничены %i.", ThreadCount, MaxThreadCount);
        ThreadCount = MaxThreadCount;
    }

    // Инициализируйте систему заданий.
    // Требует знания поддержки многопоточности рендеринга, поэтому следует инициализировать здесь.
    u32 JobThreadTypes[15];
    for (u32 i = 0; i < 15; ++i) {
        JobThreadTypes[i] = Job::General;
    }

    if (MaxThreadCount == 1 || !RendererMultithreaded) {
        // Все в одном потоке заданий.
        JobThreadTypes[0] |= (Job::GPUResource | Job::ResourceLoad);
    } else if (MaxThreadCount == 2) {
        // Разделите вещи между 2 потоками
        JobThreadTypes[0] |= Job::GPUResource;
        JobThreadTypes[1] |= Job::ResourceLoad;
    } else {
        // Выделите первые 2 потока этим вещам, передайте общие задачи другим потокам.
        JobThreadTypes[0] = Job::GPUResource;
        JobThreadTypes[1] = Job::ResourceLoad;
    }

    JobSystem::Config JobSysConfig { ThreadCount, JobThreadTypes };
    ptr = state->SystemsAllocator.Allocate(sizeof(JobSystem));
    JobSystem::Initialize(ptr, JobSysConfig);
    if (!state->Register(Job, ptr, JobSystem::Update)) {
        MERROR("Не удалось зарегистрировать систему задач.");
        return false;
    }
    #pragma endregion

    return true;
}

bool SystemsManager::RegisterKnownSystemsPostBoot(ApplicationConfig &AppConfig)
{
    void* ptr = nullptr;
    u64 MemoryRequirement = 0;

    #pragma region Система текстур.
    TextureSystemConfig TextureSysConfig{ 65536 };
    TextureSystem::Initialize(MemoryRequirement, ptr = nullptr, TextureSysConfig);
    ptr = state->SystemsAllocator.Allocate(MemoryRequirement);
    if (!Register(Texture, TextureSystem::Initialize(MemoryRequirement, ptr, TextureSysConfig))) {
        MERROR("Не удалось зарегистрировать систему текстур.");
        return false;
    }
    #pragma endregion

    #pragma region Система шрифтов.
    FontSystem::Initialize(MemoryRequirement,  ptr = nullptr, AppConfig.FontConfig);
    ptr = state->SystemsAllocator.Allocate(MemoryRequirement);
    if (!Register(Font, FontSystem::Initialize(MemoryRequirement, ptr, AppConfig.FontConfig))) {
        MERROR("Не удалось зарегистрировать систему шрифтов.");
        return false;
    }
    #pragma endregion

    #pragma region Камера
    CameraSystemConfig CameraSysConfig{ 61 };
    CameraSystem::Initialize(MemoryRequirement, ptr = nullptr, CameraSysConfig);
    ptr = state->SystemsAllocator.Allocate(MemoryRequirement);
    if (!Register(Camera, CameraSystem::Initialize(MemoryRequirement, ptr, CameraSysConfig))) {
        MERROR("Не удалось зарегистрировать систему камер.");
        return false;
    }
    #pragma endregion

    #pragma region Система материалов.
    MaterialSystemConfig MaterialSysConfig{ 4096 };
    MaterialSystem::Initialize(MemoryRequirement, ptr = nullptr, MaterialSysConfig);
    ptr = state->SystemsAllocator.Allocate(MemoryRequirement);
    if (!Register(Material, MaterialSystem::Initialize(MemoryRequirement, ptr, MaterialSysConfig))) {
        MERROR("Не удалось зарегистрировать систему материалов.");
        return false;
    }
    #pragma endregion

    #pragma region Система геометрий.
    GeometrySystemConfig GeometrySysConfig{ 4096 };
    GeometrySystem::Initialize(MemoryRequirement, ptr = nullptr, GeometrySysConfig);
    ptr = state->SystemsAllocator.Allocate(MemoryRequirement);
    if (!Register(Geometry, GeometrySystem::Initialize(MemoryRequirement, ptr, GeometrySysConfig))) {
        MERROR("Не удалось зарегистрировать систему геометрий.");
        return false;
    }
    #pragma endregion

    #pragma region Система освещения.
    if (!Register(Light, new(state->SystemsAllocator.Allocate(sizeof(LightSystem))) LightSystem())) {
        MERROR("Не удалось зарегистрировать систему освещения.");
        return false;
    }
    #pragma endregion

    return true;
}

void SystemsManager::ShutdownKnownSystems()
{
    systems[Camera].state = nullptr;
    ((FontSystem*)systems[Font].state)->Shutdown();

    systems[Geometry].state = nullptr;
    ((MaterialSystem*)systems[Material].state)->Shutdown();
    ((TextureSystem*)systems[Texture].state)->Shutdown();

    ((JobSystem*)systems[Job].state)->Shutdown();
    ((ShaderSystem*)systems[Shader].state)->Shutdown();
    ((RenderingSystem*)systems[Renderer].state)->Shutdown();

    systems[Resource].state = nullptr;
    ((PlatformState*)systems[Platform].state)->Shutdown();
    systems[Input].state   = nullptr;
    systems[Logging].state = nullptr;
    systems[Event].state   = nullptr;
    systems[MVar].state    = nullptr;
    systems[Console].state = nullptr;

    for (u32 i = ExtensionMax; i < Max; i++) {
        auto system = systems[i];
        if (system.Shutdown) {
            system.Shutdown(system.state);
        }
        system.state = nullptr;
    }
    
}
