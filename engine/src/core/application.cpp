#include "application.hpp"
#include "game_types.hpp"

ApplicationState* Application::AppState;

bool Application::ApplicationCreate(GameTypes *GameInst)
{
    if (GameInst->State) {
        MERROR("ApplicationCreate вызывался более одного раза.");
        return false;
    }

    GameInst->State->AppState = MMemory::TAllocate<ApplicationState>(1, MEMORY_TAG_APPLICATION);
    AppState = GameInst->State->AppState;
    AppState->GameInst = GameInst;
    AppState->IsRunning = false;
    AppState->IsSuspended = false;

    u64 SystemsAllocatorTotalSize = 64 * 1024 * 1024;  // 64 mb
    AppState->SystemAllocator = LinearAllocator(SystemsAllocatorTotalSize);

    AppState->mem = new MMemory();

    // Инициализируйте подсистемы.
    AppState->logger = new Logger();
    if (!AppState->logger->Initialize()) { // AppState->logger->Initialize();
        MERROR("Не удалось инициализировать систему ведения журнала; завершение работы.");
        return false;
    }

    AppState->Inputs = new Input();

    AppState->IsRunning = true;
    AppState->IsSuspended = false;

    AppState->Events = new Event();
    if (!AppState->Events->Initialize()) {
        MERROR("Система событий не смогла инициализироваться. Приложение не может быть продолжено.");
        return false;
    }

    Event::Register(EVENT_CODE_APPLICATION_QUIT, nullptr, ApplicationOnEvent);
    Event::Register(EVENT_CODE_KEY_PRESSED, nullptr, ApplicationOnKey);
    Event::Register(EVENT_CODE_KEY_RELEASED, nullptr, ApplicationOnKey);
    Event::Register(EVENT_CODE_RESIZED, nullptr, ApplicationOnResized);
    
    AppState->Window = new MWindow(GameInst->AppConfig.name,
                        GameInst->AppConfig.StartPosX, 
                        GameInst->AppConfig.StartPosY, 
                        GameInst->AppConfig.StartHeight, 
                        GameInst->AppConfig.StartWidth);

    if (!AppState->Window)
    {
        return false;
    }
    else AppState->Window->Create();

    AppState->Render = new Renderer();
    // Запуск рендерера
    if (!AppState->Render->Initialize(AppState->Window, GameInst->AppConfig.name, RENDERER_TYPE_VULKAN)) {
        MFATAL("Не удалось инициализировать средство визуализации. Прерывание приложения.");
        return FALSE;
    }

    // Инициализируйте игру.
    if (!GameInst->Initialize()) {
        MFATAL("Не удалось инициализировать игру.");
        return false;
    }

    GameInst->OnResize(AppState->width, AppState->height);

    return true;
}

bool Application::ApplicationRun() {
    AppState->clock.Start();
    AppState->clock.Update();
    AppState->clock.StartTime = AppState->clock.elapsed;
    f64 RunningTime = 0;
    u8 FrameCount = 0;
    f64 TargetFrameSeconds = 1.0f / 60;

    MINFO(MMemory::GetMemoryUsageStr());

    while (AppState->IsRunning) {
        if(!AppState->Window->Messages()) {
            AppState->IsRunning = false;
        }

        if(!AppState->IsSuspended) {
            // Обновите часы и получите разницу во времени.
                AppState->clock.Update();
                f64 CurrentTime = AppState->clock.elapsed;
                f64 delta = (CurrentTime - AppState->LastTime);
                f64 FrameStartTime = MWindow::PlatformGetAbsoluteTime();

            if (!AppState->GameInst->Update(delta)) {
                MFATAL("Ошибка обновления игры, выключение.");
                AppState->IsRunning = false;
                break;
            }

            // Вызовите процедуру рендеринга игры.
            if (!AppState->GameInst->Render(delta)) {
                MFATAL("Ошибка рендеринга игры, выключение.");
                AppState->IsRunning = false;
                break;
            }

            // TODO: refactor packet creation
            RenderPacket packet;
            packet.DeltaTime = delta;
            AppState->Render->DrawFrame(&packet);

            // Выясните, сколько времени занял кадр и, если ниже
            f64 FrameEndTime = MWindow::PlatformGetAbsoluteTime();
            f64 FrameElapsedTime = FrameEndTime - FrameStartTime;
            RunningTime += FrameElapsedTime;
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
            AppState->Inputs->InputUpdate(delta);

            // Update last time
            AppState->LastTime = CurrentTime;
        }
    }

    AppState->IsRunning = false;

    // Отключение системы событий.
    Event::Unregister(EVENT_CODE_APPLICATION_QUIT, nullptr, ApplicationOnEvent);
    Event::Unregister(EVENT_CODE_KEY_PRESSED, nullptr, ApplicationOnKey);
    Event::Unregister(EVENT_CODE_KEY_RELEASED, nullptr, ApplicationOnKey);
    Event::Unregister(EVENT_CODE_RESIZED, nullptr, ApplicationOnResized);
    AppState->Events->Shutdown();
    AppState->Inputs->~Input(); // ShutDown
    AppState->Render->Shutdown();

    AppState->Window->Close();
    AppState->logger->Shutdown();
    AppState->mem->Shutdown();

    return true;
}

void Application::ApplicationGetFramebufferSize(u32 & width, u32 & height)
{
    width = AppState->width;
    height = AppState->height;
}

void *Application::operator new(u64 size)
{
    return MMemory::Allocate(size, MEMORY_TAG_APPLICATION);
}

void Application::operator delete(void *ptr)
{
    return MMemory::Free(ptr, sizeof(Application), MEMORY_TAG_APPLICATION);
}

bool Application::ApplicationOnEvent(u16 code, void *sender, void *ListenerInst, EventContext context)
{
    switch (code) {
        case EVENT_CODE_APPLICATION_QUIT: {
            MINFO("Получено событие EVENT_CODE_APPLICATION_QUIT, завершение работы.\n");
            AppState->IsRunning = false;
            return true;
        }
    }

    return false;
}

bool Application::ApplicationOnKey(u16 code, void *sender, void *ListenerInst, EventContext context)
{
    if (code == EVENT_CODE_KEY_PRESSED) {
        u16 KeyCode = context.data.u16[0];
        if (KeyCode == KEY_ESCAPE) {
            // ПРИМЕЧАНИЕ. Технически событие генерируется само по себе, но могут быть и другие прослушиватели.
            EventContext data = {};
            Event::Fire(EVENT_CODE_APPLICATION_QUIT, 0, data);

            // Заблокируйте что-либо еще от обработки этого.
            return true;
        } else if (KeyCode == KEY_A) {
            // Пример проверки ключа
            MDEBUG("Явно — клавиша A нажата!");
        } else {
            MDEBUG("'%c' нажатие клавиши в окне.", KeyCode);
        }
    } else if (code == EVENT_CODE_KEY_RELEASED) {
        u16 KeyCode = context.data.u16[0];
        if (KeyCode == KEY_B) {
            // Пример проверки ключа
            MDEBUG("Явно — клавиша B отущена!");
        } else {
            MDEBUG("'%c' клавиша отпущена в окне.", KeyCode);
        }
    }

    return false;
}

bool Application::ApplicationOnResized(u16 code, void *sender, void *ListenerInst, EventContext context)
{
    if (code == EVENT_CODE_RESIZED) {
        u16 width = context.data.u16[0];
        u16 height = context.data.u16[1];

        // Проверьте, отличается ли это. Если да, запустите событие изменения размера.
        if (width != AppState->width || height != AppState->height) {
            AppState->width = width;
            AppState->height = height;

            MDEBUG("Изменение размера окна: %i, %i", width, height);

            // Обработка сворачивания
            if (width == 0 || height == 0) {
                MINFO("Окно свернуто, приложение приостанавливается.");
                AppState->IsSuspended = true;
                return true;
            } else {
                if (AppState->IsSuspended) {
                    MINFO("Окно восстановлено, возобновляется приложение.");
                    AppState->IsSuspended = false;
                }
                AppState->GameInst->OnResize(width, height);
                AppState->Render->OnResized(width, height);
            }
        }
    }

    return false;
}
