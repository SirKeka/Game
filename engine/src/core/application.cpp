#include "application.hpp"
//#include "memory/linear_allocator.hpp"
#include "game_types.hpp"
#include "renderer/renderer.hpp"
#include "systems/texture_system.hpp"
#include "systems/material_system.hpp"
#include "systems/geometry_system.hpp"

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
    LinearAllocator::Instance().Initialize(SystemsAllocatorTotalSize);

    AppState->mem = new MMemory();

    // Инициализируйте подсистемы.
    AppState->logger = new Logger();
    if (!AppState->logger->Initialize()) { // AppState->logger->Initialize();
        MERROR("Не удалось инициализировать систему ведения журнала; завершение работы.");
        return false;
    }

    Input::Instance()->Initialize();

    AppState->IsRunning = true;
    AppState->IsSuspended = false;

    //AppState->Events = new Event();
    if (!Event::GetInstance()->Initialize()) {
        MERROR("Система событий не смогла инициализироваться. Приложение не может быть продолжено.");
        return false;
    }

    Event::GetInstance()->Register(EVENT_CODE_APPLICATION_QUIT, nullptr, OnEvent);
    Event::GetInstance()->Register(EVENT_CODE_KEY_PRESSED, nullptr, OnKey);
    Event::GetInstance()->Register(EVENT_CODE_KEY_RELEASED, nullptr, OnKey);
    Event::GetInstance()->Register(EVENT_CODE_RESIZED, nullptr, OnResized);
    //TODO: временно
    Event::GetInstance()->Register(EVENT_CODE_DEBUG0, nullptr, OnDebugEvent);
    //TODO: временно
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

    // Система текстур.
    TextureSystem::SetMaxTextureCount(65536);
    if (!TextureSystem::Instance()->Initialize()) {
        MFATAL("Не удалось инициализировать систему текстур. Приложение не может быть продолжено.");
        return false;
    }

    // Система материалов
    MaterialSystem::SetMaxMaterialCount(4096);
    if (!MaterialSystem::Instance()->Initialize()) {
        MFATAL("Не удалось инициализировать систему материалов. Приложение не может быть продолжено.");
        return false;
    }

    // Система геометрии
    GeometrySystem::SetMaxGeometryCount(4096);
    if (!GeometrySystem::Instance()->Initialize()) {
        MFATAL("Не удалось инициализировать систему геометрии. Приложение не может быть продолжено.");
        return false;
    }
    
    // TODO: временно

    // Загрузите конфигурацию плоскости и загрузите из нее геометрию.
    //GeometryConfig gConfig = GeometrySystem::Instance()->GeneratePlaneConfig(10.0f, 5.0f, 5, 5, 5.0f, 2.0f, "test geometry", "test_material");
    //AppState->TestGeometry = GeometrySystem::Instance()->Acquire(gConfig, true);

    // Очистите места для конфигурации геометрии.
    //MMemory::Free(gConfig.vertices, sizeof(Vertex3D) * gConfig.VertexCount, MEMORY_TAG_ARRAY);
    //MMemory::Free(gConfig.indices, sizeof(u32) * gConfig.IndexCount, MEMORY_TAG_ARRAY);

    // Загрузите геометрию по умолчанию.
    AppState->TestGeometry = GeometrySystem::Instance()->GetDefault();
    // TODO: временно 

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
    AppState->LastTime = AppState->clock.elapsed;
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

            // TODO: временно
            GeometryRenderData TestRender;
            TestRender.gid = AppState->TestGeometry;
            TestRender.model = Matrix4D::MakeIdentity();

            packet.GeometryCount = 1;
            packet.geometries = &TestRender;
            // TODO: временно

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
            Input::Instance()->Update(delta);

            // Update last time
            AppState->LastTime = CurrentTime;
        }
    }

    AppState->IsRunning = false;

    // Отключение системы событий.
    Event::GetInstance()->Unregister(EVENT_CODE_APPLICATION_QUIT, nullptr, OnEvent);
    Event::GetInstance()->Unregister(EVENT_CODE_KEY_PRESSED, nullptr, OnKey);
    Event::GetInstance()->Unregister(EVENT_CODE_KEY_RELEASED, nullptr, OnKey);
    Event::GetInstance()->Unregister(EVENT_CODE_RESIZED, nullptr, OnResized);
    //TODO: временно
    Event::GetInstance()->Unregister(EVENT_CODE_DEBUG0, nullptr, OnDebugEvent);
    //TODO: временно
    Event::GetInstance()->Shutdown();
    Input::Instance()->Sutdown();
    GeometrySystem::Instance()->Shutdown();
    MaterialSystem::Instance()->Shutdown();
    TextureSystem::Instance()->Shutdown();
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

bool Application::OnEvent(u16 code, void *sender, void *ListenerInst, EventContext context)
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

bool Application::OnKey(u16 code, void *sender, void *ListenerInst, EventContext context)
{
    if (code == EVENT_CODE_KEY_PRESSED) {
        u16 KeyCode = context.data.u16[0];
        if (KeyCode == KEY_ESCAPE) {
            // ПРИМЕЧАНИЕ. Технически событие генерируется само по себе, но могут быть и другие прослушиватели.
            EventContext data = {};
            Event::GetInstance()->Fire(EVENT_CODE_APPLICATION_QUIT, 0, data);

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

bool Application::OnResized(u16 code, void *sender, void *ListenerInst, EventContext context)
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

// TODO: временно
bool Application::OnDebugEvent(u16 code, void *sender, void *ListenerInst, EventContext context)
{
    const char* names[3] = {
        "asphalt",
        "iris",
        "uvgrid"};
    static i8 choice = 2;

    // Сохраните старое имя.
    MString OldName = names[choice];

    choice++;
    choice %= 3;

    // Приобретите новую текстуру.
    if (AppState->TestGeometry) {
        AppState->TestGeometry->material->DiffuseMap.texture = TextureSystem::Instance()->Acquire(names[choice], true);
        if (!AppState->TestGeometry->material->DiffuseMap.texture) {
            MWARN("Event::OnDebugEvent нет текстуры! используется значение по умолчанию");
            AppState->TestGeometry->material->DiffuseMap.texture = TextureSystem::Instance()->GetDefaultTexture();
        }

        // Освободите старую текстуру.
        TextureSystem::Instance()->Release(OldName.c_str());
    }

    return true;
}
//TODO: временно
