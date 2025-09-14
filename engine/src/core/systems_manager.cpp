#include "systems_manager.hpp"

#include "core/console.hpp"
#include "core/engine.h"
#include "core/event.h"
#include "core/memory_system.h"
#include "core/mvar.hpp"
#include "core/input.hpp"
#include "platform/platform.hpp"

#include "renderer/rendering_system.h"

#include "systems/camera_system.hpp"
#include "systems/geometry_system.h"
#include "systems/job_systems.hpp"
#include "systems/light_system.h"
#include "systems/render_view_system.h"
#include "systems/resource_system.h"
#include "systems/shader_system.h"
#include "systems/texture_system.h"

#include <new>

SystemsManager* SystemsManager::state = nullptr;

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

bool SystemsManager::Update(const FrameData& rFrameData)
{
    for (u32 i = 0; i < M_SYSTEM_TYPE_MAX_COUNT; ++i) {
        auto& s = systems[i];
        if (s.update) {
            if (!s.update(s.state, rFrameData)) {
                MERROR("Ошибка обновления системы для типа: %i", i);
            }
        }
    }
    return true;
}

bool SystemsManager::Register(u16 type, PFN_SystemInitialize initialize, PFN_SystemShutdown shutdown, PFN_SystemUpdate update, void *config)
{
    MSystem sys;
    sys.initialize = initialize;
    sys.shutdown = shutdown;
    sys.update = update;

    // Вызов инициализации, выделение памяти, повторный вызов инициализации с выделенным блоком.
    if (sys.initialize) {
        if (!sys.initialize(sys.StateSize, nullptr, config)) {
            MERROR("Не удалось зарегистрировать систему — инициализация не удалась.");
            return false;
        }

        sys.state = SystemsAllocator.Allocate(sys.StateSize);

        if (!sys.initialize(sys.StateSize, sys.state, config)) {
            MERROR("Не удалось зарегистрировать систему — инициализация не удалась.");
            return false;
        }
    } else {
        if (type != MSystem::Memory) {
            MERROR("Инициализация требуется для типов, кроме MSystem::Memory.");
            return false;
        }
    }

    systems[type] = sys;

    return true;
}

void *SystemsManager::GetState(u16 type)
{
    if (state) {
        return state->systems[type].state;
    }
    
    return nullptr;
}

bool SystemsManager::RegisterKnownSystemsPreBoot(ApplicationConfig &AppConfig)
{
    // Память
    if (!state->Register(MSystem::Memory, nullptr, MemorySystem::Shutdown)) {
        MERROR("Не удалось зарегистрировать систему памяти.");
        return false;
    }

    // Консоль
    if (!state->Register(MSystem::Console, Console::Initialize, Console::Shutdown)) {
        MERROR("Не удалось зарегистрировать консольную систему.");
        return false;
    }

    // MVars
    if (!state->Register(MSystem::MVar, MVar::Initialize, MVar::Shutdown)) {
        MERROR("Не удалось зарегистрировать систему KVar.");
        return false;
    }

    // События
    if (!state->Register(MSystem::Event, EventSystem::Initialize, EventSystem::Shutdown)) {
        MERROR("Не удалось зарегистрировать систему событий.");
        return false;
    }

    // Журнал
    if (!state->Register(MSystem::Logging, Log::Initialize, Log::Shutdown)) {
        MERROR("Не удалось зарегистрировать систему логирования.");
        return false;
    }

    // Ввод
    if (!state->Register(MSystem::Input, InputSystem::Initialize, InputSystem::Shutdown)) {
        MERROR("Не удалось зарегистрировать систему ввода.");
        return false;
    }

    // Оконная система
    if (!state->Register(MSystem::Platform, WindowSystem::Initialize, WindowSystem::Shutdown, nullptr, &AppConfig)) {
        MERROR("Не удалось зарегистрировать систему окон.");
        return false;
    }

    // Система ресурсов.
    ResourceSystemConfig ResourceSysConfig;
    ResourceSysConfig.AssetBasePath = "../assets";  // ЗАДАЧА: Вероятно, приложение должно это настроить.
    ResourceSysConfig.MaxLoaderCount = 32;
    if (!state->Register(MSystem::Resource, ResourceSystem::Initialize, ResourceSystem::Shutdown, nullptr, &ResourceSysConfig)) {
        MERROR("Не удалось зарегистрировать систему ресурсов.");
        return false;
    }

    // Система шейдеров
    ShaderSystem::Config ShaderSysConfig;
    ShaderSysConfig.MaxShaderCount = 1024;
    ShaderSysConfig.MaxUniformCount = 128;
    ShaderSysConfig.MaxGlobalTextures = 31;
    ShaderSysConfig.MaxInstanceTextures = 31;
    if (!state->Register(MSystem::Shader, ShaderSystem::Initialize, ShaderSystem::Shutdown, nullptr, &ShaderSysConfig)) {
        MERROR("Не удалось зарегистрировать систему шейдеров.");
        return false;
    }

    // Система отрисовки
    RenderingSystemConfig RendererSysConfig = {0};
    RendererSysConfig.ApplicationName = AppConfig.name;
    RendererSysConfig.plugin = AppConfig.plugin;
    if (!state->Register(MSystem::Renderer, RenderingSystem::Initialize, RenderingSystem::Shutdown, nullptr, &RendererSysConfig)) {
        MERROR("Не удалось зарегистрировать систему отрисовки.");
        return false;
    }

    bool RendererMultithreaded = RenderingSystem::IsMultithreaded();

    // Это действительно количество ядер. Вычтите 1, чтобы учесть, что основной поток уже используется.
    i32 ThreadCount = PlatformGetProcessorCount() - 1;
    if (ThreadCount < 1) {
        MFATAL("Ошибка: платформа сообщила количество процессоров (минус один для основного потока) как %i. Нужен как минимум один дополнительный поток для системы заданий.", ThreadCount);
        return false;
    } else {
        MTRACE("Доступные потоки: %i", ThreadCount);
    }

    // Ограничьте количество потоков.
    const i32 MaxThreadCount = 15;
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

    JobSystemConfig JobSysConfig = {0};
    JobSysConfig.MaxJobThreadCount = ThreadCount;
    JobSysConfig.TypeMasks = JobThreadTypes;
    if (!state->Register(MSystem::Job, JobSystem::Initialize, JobSystem::Shutdown, JobSystem::Update, &JobSysConfig)) {
        MERROR("Не удалось зарегистрировать систему задач.");
        return false;
    }

    return true;
}

bool SystemsManager::RegisterKnownSystemsPostBoot(ApplicationConfig &AppConfig)
{
    // Система текстур.
    TextureSystemConfig TextureSysConfig;
    TextureSysConfig.MaxTextureCount = 65536;
    if (!Register(MSystem::Texture, TextureSystem::Initialize, TextureSystem::Shutdown, nullptr, &TextureSysConfig)) {
        MERROR("Не удалось зарегистрировать систему текстур.");
        return false;
    }

    // Система шрифтов.
    if (!Register(MSystem::Font, FontSystem::Initialize, FontSystem::Shutdown, nullptr, &AppConfig.FontConfig)) {
        MERROR("Не удалось зарегистрировать систему шрифтов.");
        return false;
    }

    // Камера
    CameraSystemConfig CameraSysConfig;
    CameraSysConfig.MaxCameraCount = 61;
    if (!Register(MSystem::Camera, CameraSystem::Initialize, CameraSystem::Shutdown, nullptr, &CameraSysConfig)) {
        MERROR("Не удалось зарегистрировать систему камер.");
        return false;
    }

    RenderViewSystemConfig RenderViewSysConfig = {};
    RenderViewSysConfig.MaxViewCount = 251;
    if (!Register(MSystem::RendererView, RenderViewSystem::Initialize, RenderViewSystem::Shutdown, nullptr, &RenderViewSysConfig)) {
        MERROR("Не удалось зарегистрировать систему render view.");
        return false;
    }

    // Загрузить RenderView из конфигурации приложения.
    const u32& ViewCount = AppConfig.RenderViews.Length();
    for (u32 v = 0; v < ViewCount; ++v) {
        auto view = &AppConfig.RenderViews[v];
        if (!RenderViewSystem::Register(view)) {
            MFATAL("Не удалось зарегистрировать представление '%s'. Отмена приложения.", view->name);
            return false;
        }
    }

    // Система материалов.
    MaterialSystemConfig MaterialSysConfig;
    MaterialSysConfig.MaxMaterialCount = 4096;
    if (!Register(MSystem::Material, MaterialSystem::Initialize, MaterialSystem::Shutdown, nullptr, &MaterialSysConfig)) {
        MERROR("Не удалось зарегистрировать систему материалов.");
        return false;
    }

    // Система геометрий.
    GeometrySystemConfig GeometrySysConfig;
    GeometrySysConfig.MaxGeometryCount = 4096;
    if (!Register(MSystem::Geometry, GeometrySystem::Initialize, GeometrySystem::Shutdown, nullptr, &GeometrySysConfig)) {
        MERROR("Не удалось зарегистрировать систему геометрий.");
        return false;
    }

    // Система освещения.
    if (!Register(MSystem::Light, LightSystem::Initialize, LightSystem::Shutdown)) {
        MERROR("Не удалось зарегистрировать систему освещения.");
        return false;
    }

    return true;
}

void SystemsManager::ShutdownKnownSystems()
{
    systems[MSystem::Type::Camera].shutdown();
    systems[MSystem::Type::Font].shutdown();

    systems[MSystem::Type::RendererView].shutdown();
    systems[MSystem::Type::Geometry].shutdown();
    systems[MSystem::Type::Material].shutdown();
    systems[MSystem::Type::Texture].shutdown();

    systems[MSystem::Type::Job].shutdown();
    systems[MSystem::Type::Shader].shutdown();
    systems[MSystem::Type::Renderer].shutdown();

    systems[MSystem::Resource].shutdown();
    systems[MSystem::Platform].shutdown();
    systems[MSystem::Input].shutdown();
    systems[MSystem::Logging].shutdown();
    systems[MSystem::Event].shutdown();
    systems[MSystem::MVar].shutdown();
    systems[MSystem::Console].shutdown();
}
