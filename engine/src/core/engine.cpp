#include "engine.hpp"
#include "version.hpp"

//#include "memory/linear_allocator.hpp"
#include "application_types.hpp"
#include "renderer/renderer.hpp"

#include "console.hpp"

// Системы
#include "systems/texture_system.hpp"
#include "systems/material_system.hpp"
#include "systems/geometry_system.hpp"
#include "systems/resource_system.hpp"
#include "systems/shader_system.hpp"
#include "systems/camera_system.hpp"
#include "systems/render_view_system.hpp"
#include "systems/job_systems.hpp"
#include "systems/font_system.hpp"

#include "renderer/views/render_view_pick.hpp"

// Компоненты ядра движка
#include "uuid.hpp"

#include <new>

Engine* Engine::pEngine = nullptr;

constexpr Engine::Engine() 
: 
SystemAllocator(),
logger(),
Events(),
IsRunning(),
IsSuspended(),
Window(),
Render(),
GameInst(),
JobSystemInst(),
RenderViewSystemInst(),
ResourceSystemInst(),
width(),
height(),
clock(),
LastTime() {}

bool Engine::Create(Application *GameInst)
{
    if (GameInst->engine) {
        MERROR("Engine::Create вызывался более одного раза.");
        return false;
    }

    // Система памяти должна быть установлена в первую очередь. 
    //State->mem = new MMemory();
    if (!MMemory::Initialize(GIBIBYTES(1))) {
        MERROR("Не удалось инициализировать систему памяти; Выключение.");
        return false;
    }

    // Начальный генератор uuid.
    // ЗАДАЧА: Добавить более лучший начальный генератор.
    uuid::Seed(101);

    Metrics::Initialize();

    GameInst->state = MMemory::Allocate(GameInst->StateMemoryRequirement, Memory::Engine);
    GameInst->engine = new Engine();
    pEngine = GameInst->engine;
    pEngine->GameInst = GameInst;

    u64 SystemsAllocatorTotalSize = 64 * 1024 * 1024;  // 64 mb
    pEngine->SystemAllocator.Initialize(SystemsAllocatorTotalSize);

    // Инициализируйте подсистемы.
    Console::Initialize(pEngine->SystemAllocator);

    pEngine->logger = reinterpret_cast<Logger*>(pEngine->SystemAllocator.Allocate(sizeof(Logger)));
    if (!pEngine->logger->Initialize()) { // State->logger->Initialize();
        MERROR("Не удалось инициализировать систему ведения журнала; завершение работы.");
        return false;
    }

    Input::Initialize(pEngine->SystemAllocator);

    pEngine->IsRunning = true;
    pEngine->IsSuspended = false;
    
    if (!Event::Initialize()) {
        MERROR("Система событий не смогла инициализироваться. Приложение не может быть продолжено.");
        return false;
    }
    pEngine->Events = Event::GetInstance();

    pEngine->Events->Register(EVENT_CODE_APPLICATION_QUIT, nullptr, OnEvent);
    pEngine->Events->Register(EVENT_CODE_RESIZED, nullptr, OnResized);
    pEngine->Events->Register(EVENT_CODE_OBJECT_HOVER_ID_CHANGED, nullptr, OnEvent);

    pEngine->Window = new MWindow(GameInst->AppConfig.name,
                        GameInst->AppConfig.StartPosX, 
                        GameInst->AppConfig.StartPosY, 
                        GameInst->AppConfig.StartHeight, 
                        GameInst->AppConfig.StartWidth);

    if (!pEngine->Window) {
        return false;
    } else pEngine->Window->Create();

    // Система ресурсов
    if (!ResourceSystem::Initialize(32, "../assets", pEngine->SystemAllocator)) {
        MFATAL("Не удалось инициализировать систему ресурсов. Приложение не может быть продолжено.");
        return false;
    }
    pEngine->ResourceSystemInst = ResourceSystem::Instance();

    // Система шейдеров
    ShaderSystem::Config ShaderSysConfig;
    ShaderSysConfig.MaxShaderCount      = 1024;
    ShaderSysConfig.MaxUniformCount     =  128;
    ShaderSysConfig.MaxGlobalTextures   =   31;
    ShaderSysConfig.MaxInstanceTextures =   31;
    if(!ShaderSystem::Initialize(ShaderSysConfig, pEngine->SystemAllocator)) {
        MFATAL("Не удалось инициализировать шейдерную систему. Прерывание приложения.");
        return false;
    }
    
    void* ptrRendererMem = pEngine->SystemAllocator.Allocate(sizeof(Renderer));
    pEngine->Render = new(ptrRendererMem) Renderer(pEngine->Window, GameInst->AppConfig.name, ERendererType::VULKAN, pEngine->SystemAllocator);
    // Запуск рендерера
    if (!pEngine->Render) {
        MFATAL("Не удалось инициализировать средство визуализации. Прерывание приложения.");
        return false;
    }
    bool RendererMultithreaded = pEngine->Render->IsMultithreaded();

    if (!pEngine->GameInst->Boot()) {
        MFATAL("Game::Boot - Сбой последовательности загрузки игры; прерывание работы приложения.");
        return false;
    }

    // Сообщить о версии движка
    MINFO("Moon Engine v. %s", MVERSION);

    // Это действительно количество ядер. Вычтите 1, чтобы учесть, что основной поток уже используется.
    i32 ThreadCount = PlatformGetProcessorCount() - 1;
    if (ThreadCount < 1) {
        MFATAL("Ошибка: платформа сообщила количество процессоров (минус один для основного потока) как %i. Нужен как минимум один дополнительный поток для системы заданий.", ThreadCount);
        return false;
    } else {
        MTRACE("Доступные потоки: %i", ThreadCount);
    }

    // Ограничьте количество нитей.
    const i32 MaxThreadCount = 15;
    if (ThreadCount > MaxThreadCount) {
        MTRACE("Доступные потоки в системе — %i, но будут ограничены %i.", ThreadCount, MaxThreadCount);
        ThreadCount = MaxThreadCount;
    }

    // Инициализируйте систему заданий.
    // Требует знания поддержки многопоточности рендеринга, поэтому следует инициализировать здесь.
    u32 JobThreadTypes[15];
    for (u32 i = 0; i < 15; ++i) {
        JobThreadTypes[i] = JobType::General;
    }

    if (MaxThreadCount == 1 || !RendererMultithreaded) {
        // Все в одном потоке заданий.
        JobThreadTypes[0] |= (JobType::GPUResource | JobType::ResourceLoad);
    } else if (MaxThreadCount == 2) {
        // Разделите вещи между 2 потоками
        JobThreadTypes[0] |= JobType::GPUResource;
        JobThreadTypes[1] |= JobType::ResourceLoad;
    } else {
        // Выделите первые 2 потока этим вещам, передайте общие задачи другим потокам.
        JobThreadTypes[0] = JobType::GPUResource;
        JobThreadTypes[1] = JobType::ResourceLoad;
    }

    // Система заданий(потоков)
    if (!JobSystem::Initialize(ThreadCount, JobThreadTypes, pEngine->SystemAllocator)) {
        MFATAL("Не удалось инициализировать систему заданий. Отмена приложения.");
        return false;
    }
    pEngine->JobSystemInst = JobSystem::Instance();

    // Система текстур.
    if (!TextureSystem::Initialize(65536, pEngine->SystemAllocator)) {
        MFATAL("Не удалось инициализировать систему текстур. Приложение не может быть продолжено.");
        return false;
    }

    // Система шрифтов.
    if (!FontSystem::Initialize(pEngine->GameInst->AppConfig.FontConfig, pEngine->SystemAllocator)) {
        MFATAL("Не удалось инициализировать систему шрифтов. Приложение не может продолжать работу.");
        return false;
    }

    // Система камер
    if (!CameraSystem::Initialize(61, pEngine->SystemAllocator)) {
        MFATAL("Не удалось инициализировать систему камер. Приложение не может быть продолжено.");
        return false;
    }
    
    // Система визуадизаций
    if (!RenderViewSystem::Initialize(251, pEngine->SystemAllocator)) {
        MFATAL("Не удалось инициализировать систему визуализаций. Приложение не может быть продолжено.");
        return false;
    }

    // Загрузка визуализатора вида
    const auto& ViewCount = GameInst->AppConfig.RenderViews.Length();
    for(u32 i = 0; i < ViewCount; ++i) {
        auto& view = GameInst->AppConfig.RenderViews[i];
        if (!pEngine->RenderViewSystemInst->Create(view)) {
            MFATAL("Не удалось создать представление скайбокса. Отмена приложения.");
            return false;
        }
    }

    // Система материалов
    if (!MaterialSystem::Initialize(4096, pEngine->SystemAllocator)) {
        MFATAL("Не удалось инициализировать систему материалов. Приложение не может быть продолжено.");
        return false;
    }

    // Система геометрии
    if (!GeometrySystem::Initialize(4096, pEngine->SystemAllocator)) {
        MFATAL("Не удалось инициализировать систему геометрии. Приложение не может быть продолжено.");
        return false;
    }

    // Инициализируйте игру.
    if (!GameInst->Initialize()) {
        MFATAL("Не удалось инициализировать игру.");
        return false;
    }

    // Вызовите resize один раз, чтобы убедиться, что установлен правильный размер.
    pEngine->Render->OnResized(pEngine->width, pEngine->height);
    GameInst->OnResize(pEngine->width, pEngine->height);

    return true;
}

bool Engine::Run() {
    clock.Start();
    clock.Update();
    LastTime = clock.elapsed;
    // f64 RunningTime = 0;
    [[maybe_unused]]u8 FrameCount = 0;      // 
    f64 TargetFrameSeconds = 1.0f / 60;
    f64 FrameElapsedTime = 0;

    MINFO(MMemory::GetMemoryUsageStr().c_str());

    while (IsRunning) {
        if(!Window->Messages()) {
            IsRunning = false;
        }

        if(!IsSuspended) {
            // Обновите часы и получите разницу во времени.
            clock.Update();
            f64 CurrentTime = clock.elapsed;
            f64 delta = (CurrentTime - LastTime);
            f64 FrameStartTime = MWindow::PlatformGetAbsoluteTime();

            // Обновлление системы работы(потоков)
            JobSystemInst->Update();

            //Обновление метрик(статистики)
            Metrics::Update(FrameElapsedTime);

            if (!GameInst->Update(delta)) {
                MFATAL("Ошибка обновления игры, выключение.");
                IsRunning = false;
                break;
            }

            RenderPacket packet = {};
            packet.DeltaTime = delta;

            // Вызовите процедуру рендеринга игры.
            if (!GameInst->Render(packet, delta)) {
                MFATAL("Ошибка рендеринга игры, выключение.");
                IsRunning = false;
                break;
            }

            Render->DrawFrame(packet);

            // Выясните, сколько времени занял кадр и, если ниже
            f64 FrameEndTime = MWindow::PlatformGetAbsoluteTime();
            FrameElapsedTime = FrameEndTime - FrameStartTime;
            // RunningTime += FrameElapsedTime;
            f64 RemainingSeconds = TargetFrameSeconds - FrameElapsedTime;

            if (RemainingSeconds > 0) {
                u64 Remaining_ms = (RemainingSeconds * 1000);

                // Если осталось время, верните его ОС.
                bool LimitFrames = false;
                if (Remaining_ms > 0 && LimitFrames) {
                    PlatformSleep(Remaining_ms - 1);
                }

                FrameCount++;
            }

            // ПРИМЕЧАНИЕ. Обновление/копирование состояния ввода всегда
            // должно выполняться после записи любого ввода; т.е. перед этой
            // строкой. В целях безопасности входные данные обновляются в
            // последнюю очередь перед завершением этого кадра.
            Input::Instance()->Update(delta);

            // Update last time
            LastTime = CurrentTime;
        }
    }

    IsRunning = false;

    // Выключение игры
    GameInst->Shutdown();
    // Отключение системы событий.
    Events->Unregister(EVENT_CODE_APPLICATION_QUIT, nullptr, OnEvent);
    Events->Unregister(EVENT_CODE_RESIZED, nullptr, OnResized);
    Events->Shutdown(); // ЗАДАЧА: при удалении указателя на систему событий происходит ошибка
    Input::Instance()->Sutdown();
    FontSystem::Shutdown();
    RenderViewSystem::Shutdown();
    GeometrySystem::Instance()->Shutdown();
    MaterialSystem::Shutdown();
    TextureSystem::Instance()->Shutdown();

    ShaderSystem::Shutdown();
    Render->Shutdown();
    ResourceSystem::Shutdown();
    JobSystem::Shutdown();

    Window->Close();
    logger->Shutdown();
    Console::Shutdown();
    MMemory::Shutdown();

    return true;
}

void *Engine::operator new(u64 size)
{
    return MMemory::Allocate(size, Memory::Engine);
}

void Engine::operator delete(void *ptr, u64 size)
{
    return MMemory::Free(ptr, size, Memory::Engine);
}

bool Engine::OnEvent(u16 code, void *sender, void *ListenerInst, EventContext context)
{
    switch (code) {
        case EVENT_CODE_APPLICATION_QUIT: {
            MINFO("Получено событие EVENT_CODE_APPLICATION_QUIT, завершение работы.\n");
            pEngine->IsRunning = false;
            return true;
        }
    }

    return false;
}

bool Engine::OnResized(u16 code, void *sender, void *ListenerInst, EventContext context)
{
    if (code == EVENT_CODE_RESIZED) {
        u16 width = context.data.u16[0];
        u16 height = context.data.u16[1];

        // Проверьте, отличается ли это. Если да, запустите событие изменения размера.
        if (width != pEngine->width || height != pEngine->height) {
            pEngine->width = width;
            pEngine->height = height;

            MDEBUG("Изменение размера окна: %i, %i", width, height);

            // Обработка сворачивания
            if (width == 0 || height == 0) {
                MINFO("Окно свернуто, приложение приостанавливается.");
                pEngine->IsSuspended = true;
                return true;
            } else {
                if (pEngine->IsSuspended) {
                    MINFO("Окно восстановлено, возобновляется приложение.");
                    pEngine->IsSuspended = false;
                }
                pEngine->GameInst->OnResize(width, height);
                pEngine->Render->OnResized(width, height);
                return true;
            }
        }
    }
    // Событие намеренно не обрабатывается, чтобы другие слушатели могли его получить.
    return false;
}
