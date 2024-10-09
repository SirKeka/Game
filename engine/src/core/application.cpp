#include "application.hpp"
#include "version.hpp"

//#include "memory/linear_allocator.hpp"
#include "game_types.hpp"
#include "renderer/renderer.hpp"

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

ApplicationState* Application::State = nullptr;

bool Application::Create(GameTypes *GameInst)
{
    if (GameInst->application) {
        MERROR("ApplicationCreate вызывался более одного раза.");
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

    GameInst->state = MMemory::Allocate(GameInst->StateMemoryRequirement, Memory::Application);
    GameInst->application->State = MMemory::TAllocate<ApplicationState>(Memory::Application); // ЗАДАЧА: Переделать.
    State = GameInst->application->State;
    State->GameInst = GameInst;

    u64 SystemsAllocatorTotalSize = 64 * 1024 * 1024;  // 64 mb
    State->SystemAllocator.Initialize(SystemsAllocatorTotalSize);

    // Инициализируйте подсистемы.
    State->logger = reinterpret_cast<Logger*>(State->SystemAllocator.Allocate(sizeof(Logger)));
    if (!State->logger->Initialize()) { // State->logger->Initialize();
        MERROR("Не удалось инициализировать систему ведения журнала; завершение работы.");
        return false;
    }

    Input::Initialize(State->SystemAllocator);

    State->IsRunning = true;
    State->IsSuspended = false;
    
    if (!Event::Initialize()) {
        MERROR("Система событий не смогла инициализироваться. Приложение не может быть продолжено.");
        return false;
    }
    State->Events = Event::GetInstance();

    State->Events->Register(EVENT_CODE_APPLICATION_QUIT, nullptr, OnEvent);
    State->Events->Register(EVENT_CODE_RESIZED, nullptr, OnResized);
    State->Events->Register(EVENT_CODE_OBJECT_HOVER_ID_CHANGED, nullptr, OnEvent);

    State->Window = new MWindow(GameInst->AppConfig.name,
                        GameInst->AppConfig.StartPosX, 
                        GameInst->AppConfig.StartPosY, 
                        GameInst->AppConfig.StartHeight, 
                        GameInst->AppConfig.StartWidth);

    if (!State->Window) {
        return false;
    } else State->Window->Create();

    // Система ресурсов
    if (!ResourceSystem::Initialize(32, "../assets", State->SystemAllocator)) {
        MFATAL("Не удалось инициализировать систему ресурсов. Приложение не может быть продолжено.");
        return false;
    }
    State->ResourceSystemInst = ResourceSystem::Instance();

    // Система шейдеров
    ShaderSystem::Config ShaderSysConfig;
    ShaderSysConfig.MaxShaderCount      = 1024;
    ShaderSysConfig.MaxUniformCount     =  128;
    ShaderSysConfig.MaxGlobalTextures   =   31;
    ShaderSysConfig.MaxInstanceTextures =   31;
    if(!ShaderSystem::Initialize(ShaderSysConfig, State->SystemAllocator)) {
        MFATAL("Не удалось инициализировать шейдерную систему. Прерывание приложения.");
        return false;
    }
    
    void* ptrRendererMem = State->SystemAllocator.Allocate(sizeof(Renderer));
    State->Render = new(ptrRendererMem) Renderer(State->Window, GameInst->AppConfig.name, ERendererType::VULKAN, State->SystemAllocator);
    // Запуск рендерера
    if (!State->Render) {
        MFATAL("Не удалось инициализировать средство визуализации. Прерывание приложения.");
        return false;
    }
    bool RendererMultithreaded = State->Render->IsMultithreaded();

    if (!State->GameInst->Boot()) {
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
    if (!JobSystem::Initialize(ThreadCount, JobThreadTypes, State->SystemAllocator)) {
        MFATAL("Не удалось инициализировать систему заданий. Отмена приложения.");
        return false;
    }
    State->JobSystemInst = JobSystem::Instance();

    // Система текстур.
    if (!TextureSystem::Initialize(65536, State->SystemAllocator)) {
        MFATAL("Не удалось инициализировать систему текстур. Приложение не может быть продолжено.");
        return false;
    }

    // Система шрифтов.
    if (!FontSystem::Initialize(State->GameInst->AppConfig.FontConfig, State->SystemAllocator)) {
        MFATAL("Не удалось инициализировать систему шрифтов. Приложение не может продолжать работу.");
        return false;
    }

    // Система камер
    if (!CameraSystem::Initialize(61, State->SystemAllocator)) {
        MFATAL("Не удалось инициализировать систему камер. Приложение не может быть продолжено.");
        return false;
    }
    
    // Система визуадизаций
    if (!RenderViewSystem::Initialize(251, State->SystemAllocator)) {
        MFATAL("Не удалось инициализировать систему визуализаций. Приложение не может быть продолжено.");
        return false;
    }

    // Загрузка визуализатора вида
    const auto& ViewCount = GameInst->AppConfig.RenderViews.Length();
    for(u32 i = 0; i < ViewCount; ++i) {
        auto& view = GameInst->AppConfig.RenderViews[i];
        if (!State->RenderViewSystemInst->Create(view)) {
            MFATAL("Не удалось создать представление скайбокса. Отмена приложения.");
            return false;
        }
    }

    // Система материалов
    if (!MaterialSystem::Initialize(4096, State->SystemAllocator)) {
        MFATAL("Не удалось инициализировать систему материалов. Приложение не может быть продолжено.");
        return false;
    }

    // Система геометрии
    if (!GeometrySystem::Initialize(4096, State->SystemAllocator)) {
        MFATAL("Не удалось инициализировать систему геометрии. Приложение не может быть продолжено.");
        return false;
    }

    // Инициализируйте игру.
    if (!GameInst->Initialize()) {
        MFATAL("Не удалось инициализировать игру.");
        return false;
    }

    // Вызовите resize один раз, чтобы убедиться, что установлен правильный размер.
    State->Render->OnResized(State->width, State->height);
    GameInst->OnResize(State->width, State->height);

    return true;
}

bool Application::ApplicationRun() {
    State->clock.Start();
    State->clock.Update();
    State->LastTime = State->clock.elapsed;
    // f64 RunningTime = 0;
    [[maybe_unused]]u8 FrameCount = 0;      // 
    f64 TargetFrameSeconds = 1.0f / 60;
    f64 FrameElapsedTime = 0;

    MINFO(MMemory::GetMemoryUsageStr().c_str());

    while (State->IsRunning) {
        if(!State->Window->Messages()) {
            State->IsRunning = false;
        }

        if(!State->IsSuspended) {
            // Обновите часы и получите разницу во времени.
            State->clock.Update();
            f64 CurrentTime = State->clock.elapsed;
            f64 delta = (CurrentTime - State->LastTime);
            f64 FrameStartTime = MWindow::PlatformGetAbsoluteTime();

            // Обновлление системы работы(потоков)
            State->JobSystemInst->Update();

            //Обновление метрик(статистики)
            State->metrics.Update(FrameElapsedTime);

            if (!State->GameInst->Update(delta)) {
                MFATAL("Ошибка обновления игры, выключение.");
                State->IsRunning = false;
                break;
            }

            RenderPacket packet = {};
            packet.DeltaTime = delta;

            // Вызовите процедуру рендеринга игры.
            if (!State->GameInst->Render(packet, delta)) {
                MFATAL("Ошибка рендеринга игры, выключение.");
                State->IsRunning = false;
                break;
            }

            State->Render->DrawFrame(packet);

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
            State->LastTime = CurrentTime;
        }
    }

    State->IsRunning = false;

    // Выключение игры
    State->GameInst->Shutdown();
    // Отключение системы событий.
    State->Events->Unregister(EVENT_CODE_APPLICATION_QUIT, nullptr, OnEvent);
    State->Events->Unregister(EVENT_CODE_RESIZED, nullptr, OnResized);
    State->Events->Shutdown(); // ЗАДАЧА: при удалении указателя на систему событий происходит ошибка
    Input::Instance()->Sutdown();
    FontSystem::Shutdown();
    RenderViewSystem::Shutdown();
    GeometrySystem::Instance()->Shutdown();
    MaterialSystem::Shutdown();
    TextureSystem::Instance()->Shutdown();

    ShaderSystem::Shutdown();
    State->Render->Shutdown();
    ResourceSystem::Shutdown();
    JobSystem::Shutdown();

    State->Window->Close();
    State->logger->Shutdown();
    MMemory::Shutdown();

    return true;
}

void Application::ApplicationGetFramebufferSize(u32 & width, u32 & height)
{
    width = State->width;
    height = State->height;
}

void *Application::operator new(u64 size)
{
    return MMemory::Allocate(size, Memory::Application);
}

void Application::operator delete(void *ptr)
{
    return MMemory::Free(ptr, sizeof(Application), Memory::Application);
}

bool Application::OnEvent(u16 code, void *sender, void *ListenerInst, EventContext context)
{
    switch (code) {
        case EVENT_CODE_APPLICATION_QUIT: {
            MINFO("Получено событие EVENT_CODE_APPLICATION_QUIT, завершение работы.\n");
            State->IsRunning = false;
            return true;
        }
    }

    return false;
}

bool Application::OnResized(u16 code, void *sender, void *ListenerInst, EventContext context)
{
    if (code == EVENT_CODE_RESIZED) {
        u16 width = context.data.u16[0];
        u16 height = context.data.u16[1];

        // Проверьте, отличается ли это. Если да, запустите событие изменения размера.
        if (width != State->width || height != State->height) {
            State->width = width;
            State->height = height;

            MDEBUG("Изменение размера окна: %i, %i", width, height);

            // Обработка сворачивания
            if (width == 0 || height == 0) {
                MINFO("Окно свернуто, приложение приостанавливается.");
                State->IsSuspended = true;
                return true;
            } else {
                if (State->IsSuspended) {
                    MINFO("Окно восстановлено, возобновляется приложение.");
                    State->IsSuspended = false;
                }
                State->GameInst->OnResize(width, height);
                State->Render->OnResized(width, height);
                return true;
            }
        }
    }
    // Событие намеренно не обрабатывается, чтобы другие слушатели могли его получить.
    return false;
}
