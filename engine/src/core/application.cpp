#include "application.hpp"
//#include "memory/linear_allocator.hpp"
#include "game_types.hpp"
#include "renderer/renderer.hpp"
#include "systems/texture_system.hpp"
#include "systems/material_system.hpp"
#include "systems/geometry_system.hpp"
#include "systems/resource_system.hpp"
#include "systems/shader_system.hpp"

// СДЕЛАТЬ: временно
#include "math/geometry_utils.hpp"
// СДЕЛАТЬ: временно

ApplicationState* Application::State = nullptr;

bool Application::ApplicationCreate(GameTypes *GameInst)
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

    GameInst->state = MMemory::Allocate(GameInst->StateMemoryRequirement, MemoryTag::Application);
    GameInst->application->State = MMemory::TAllocate<ApplicationState>(MemoryTag::Application);
    State = GameInst->application->State;
    State->GameInst = GameInst;

    u64 SystemsAllocatorTotalSize = 64 * 1024 * 1024;  // 64 mb
    LinearAllocator::Instance().Initialize(SystemsAllocatorTotalSize);

    // Инициализируйте подсистемы.
    State->logger = new Logger();
    if (!State->logger->Initialize()) { // AppState->logger->Initialize();
        MERROR("Не удалось инициализировать систему ведения журнала; завершение работы.");
        return false;
    }

    Input::Instance()->Initialize();

    State->IsRunning = true;
    State->IsSuspended = false;

    //AppState->Events = new Event();
    if (!Event::GetInstance()->Initialize()) {
        MERROR("Система событий не смогла инициализироваться. Приложение не может быть продолжено.");
        return false;
    }

    Event::GetInstance()->Register(EVENT_CODE_APPLICATION_QUIT, nullptr, OnEvent);
    Event::GetInstance()->Register(EVENT_CODE_KEY_PRESSED, nullptr, OnKey);
    Event::GetInstance()->Register(EVENT_CODE_KEY_RELEASED, nullptr, OnKey);
    Event::GetInstance()->Register(EVENT_CODE_RESIZED, nullptr, OnResized);
    //СДЕЛАТЬ: временно
    Event::GetInstance()->Register(EVENT_CODE_DEBUG0, nullptr, OnDebugEvent);
    //СДЕЛАТЬ: временно
    State->Window = new MWindow(GameInst->AppConfig.name,
                        GameInst->AppConfig.StartPosX, 
                        GameInst->AppConfig.StartPosY, 
                        GameInst->AppConfig.StartHeight, 
                        GameInst->AppConfig.StartWidth);

    if (!State->Window) {
        return false;
    } else State->Window->Create();

    // Система ресурсов
    if (!ResourceSystem::Initialize(32, "../assets")) {
        MFATAL("Не удалось инициализировать систему ресурсов. Приложение не может быть продолжено.");
        return false;
    }

    // Система шейдеров
    if(!ShaderSystem::Initialize(1024, 128, 31, 31)) {
        MFATAL("Не удалось инициализировать шейдерную систему. Прерывание приложения.");
        return false;
    }

    State->Render = new Renderer(State->Window, GameInst->AppConfig.name, ERendererType::VULKAN);
    // Запуск рендерера
    if (!State->Render) {
        MFATAL("Не удалось инициализировать средство визуализации. Прерывание приложения.");
        return false;
    }

    // Система текстур.
    if (!TextureSystem::Initialize(65536)) {
        MFATAL("Не удалось инициализировать систему текстур. Приложение не может быть продолжено.");
        return false;
    }

    // Система материалов
    if (!MaterialSystem::Initialize(4096)) {
        MFATAL("Не удалось инициализировать систему материалов. Приложение не может быть продолжено.");
        return false;
    }

    // Система геометрии
    if (!GeometrySystem::Initialize(4096)) {
        MFATAL("Не удалось инициализировать систему геометрии. Приложение не может быть продолжено.");
        return false;
    }
    
    // СДЕЛАТЬ: временно

    // Загрузите конфигурацию плоскости и загрузите из нее геометрию.
    GeometryConfig gConfig = GeometrySystem::Instance()->GenerateCubeConfig(10.f, 10.f, 10.f, 1.f, 1.f, "test_cube", "test_material");
    Math::Geometry::GenerateTangents(gConfig.VertexCount, reinterpret_cast<Vertex3D*>(gConfig.vertices), gConfig.IndexCount, reinterpret_cast<u32*>(gConfig.indices));
    State->TestGeometry = GeometrySystem::Instance()->Acquire(gConfig, true);

    // Очистите места для конфигурации геометрии.
    MMemory::Free(gConfig.vertices, gConfig.VertexCount * sizeof(Vertex3D), MemoryTag::Array);
    MMemory::Free(gConfig.indices, gConfig.IndexCount * sizeof(u32), MemoryTag::Array);

    const f32 w = 128.f;
    const f32 h = 49.f;
    Vertex2D uiverts [4];
    uiverts[0].position.x = 0.0f;  // 0    3
    uiverts[0].position.y = 0.0f;  //
    uiverts[0].texcoord.x = 0.0f;  //
    uiverts[0].texcoord.y = 0.0f;  // 2    1

    uiverts[1].position.y = h;
    uiverts[1].position.x = w;
    uiverts[1].texcoord.x = 1.0f;
    uiverts[1].texcoord.y = 1.0f;

    uiverts[2].position.x = 0.0f;
    uiverts[2].position.y = h;
    uiverts[2].texcoord.x = 0.0f;
    uiverts[2].texcoord.y = 1.0f;

    uiverts[3].position.x = w;
    uiverts[3].position.y = 0.0;
    uiverts[3].texcoord.x = 1.0f;
    uiverts[3].texcoord.y = 0.0f;

    // Индексы - против часовой стрелки
    u32 uiindices[6] = {2, 1, 0, 3, 0, 1};
    GeometryConfig UI_Config {sizeof(Vertex2D), 4, uiverts, sizeof(u32), 6, uiindices, "test_ui_geometry", "test_ui_material"};

    // Получите геометрию пользовательского интерфейса из конфигурации.
    State->TestUI_Geometry = GeometrySystem::Instance()->Acquire(UI_Config, true);

    // Загрузите геометрию по умолчанию.
    // AppState->TestGeometry = GeometrySystem::Instance()->GetDefault();
    // СДЕЛАТЬ: временно 

    // Инициализируйте игру.
    if (!GameInst->Initialize()) {
        MFATAL("Не удалось инициализировать игру.");
        return false;
    }

    GameInst->OnResize(State->width, State->height);

    return true;
}

bool Application::ApplicationRun() {
    State->clock.Start();
    State->clock.Update();
    State->LastTime = State->clock.elapsed;
    f64 RunningTime = 0;
    u8 FrameCount = 0;
    f64 TargetFrameSeconds = 1.0f / 60;

    MINFO(MMemory::GetMemoryUsageStr());

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

            if (!State->GameInst->Update(delta)) {
                MFATAL("Ошибка обновления игры, выключение.");
                State->IsRunning = false;
                break;
            }

            // Вызовите процедуру рендеринга игры.
            if (!State->GameInst->Render(delta)) {
                MFATAL("Ошибка рендеринга игры, выключение.");
                State->IsRunning = false;
                break;
            }

            // СДЕЛАТЬ: refactor packet creation
            // СДЕЛАТЬ: временно
            static f32 angle = 0;
            angle += (.5f * delta);
            //// СДЕЛАТЬ: Что-то с матрицами вращения портит направленное освещение, в частности, кажется, по оси X. До поворота все в порядке.
            Quaternion rotation{ Vector3D<f32>(0, 1, 0), angle, true };
            //Matrix4D t = Matrix4D::MakeTranslation(Vector3D<f32>::Zero());
            //Matrix4D r = rotation;
            //Matrix4D s = Matrix4D::MakeScale(Vector3D<f32>::One()) ;
            //t = r * t;
            //t = s * t;
            GeometryRenderData TestRender{ rotation, State->TestGeometry }; //  Matrix4D(rotation, vec3_zero());
            GeometryRenderData TestUI_Render{Matrix4D::MakeTranslation(Vector3D<f32>::Zero()), State->TestUI_Geometry};

            RenderPacket packet{delta, 1, &TestRender, 1, &TestUI_Render};
            // СДЕЛАТЬ: временно

            State->Render->DrawFrame(packet);

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
            State->LastTime = CurrentTime;
        }
    }

    State->IsRunning = false;

    // Отключение системы событий.
    Event::GetInstance()->Unregister(EVENT_CODE_APPLICATION_QUIT, nullptr, OnEvent);
    Event::GetInstance()->Unregister(EVENT_CODE_KEY_PRESSED, nullptr, OnKey);
    Event::GetInstance()->Unregister(EVENT_CODE_KEY_RELEASED, nullptr, OnKey);
    Event::GetInstance()->Unregister(EVENT_CODE_RESIZED, nullptr, OnResized);
    //СДЕЛАТЬ: временно
    Event::GetInstance()->Unregister(EVENT_CODE_DEBUG0, nullptr, OnDebugEvent);
    //СДЕЛАТЬ: временно
    Event::GetInstance()->Shutdown(); // СДЕЛАТЬ: при удалении указателя на систему событий происходит ошибка
    Input::Instance()->Sutdown();
    GeometrySystem::Instance()->Shutdown();
    MaterialSystem::Shutdown();
    TextureSystem::Instance()->Shutdown();
    ShaderSystem::Shutdown();
    ResourceSystem::Instance()->Shutdown();
    State->Render->Shutdown();

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
    return MMemory::Allocate(size, MemoryTag::Application);
}

void Application::operator delete(void *ptr)
{
    return MMemory::Free(ptr, sizeof(Application), MemoryTag::Application);
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

bool Application::OnKey(u16 code, void *sender, void *ListenerInst, EventContext context)
{
    if (code == EVENT_CODE_KEY_PRESSED) {
        u16 KeyCode = context.data.u16[0];
        if (KeyCode == Keys::ESCAPE) {
            // ПРИМЕЧАНИЕ. Технически событие генерируется само по себе, но могут быть и другие прослушиватели.
            EventContext data = {};
            Event::GetInstance()->Fire(EVENT_CODE_APPLICATION_QUIT, 0, data);

            // Заблокируйте что-либо еще от обработки этого.
            return true;
        } else if (KeyCode == Keys::A) {
            // Пример проверки ключа
            MDEBUG("Явно — клавиша A нажата!");
        } else {
            MDEBUG("'%c' нажатие клавиши в окне.", KeyCode);
        }
    } else if (code == EVENT_CODE_KEY_RELEASED) {
        u16 KeyCode = context.data.u16[0];
        if (KeyCode == Keys::B) {
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
            }
        }
    }
    // Событие намеренно не обрабатывается, чтобы другие слушатели могли его получить.
    return false;
}

// TODO: временно
bool Application::OnDebugEvent(u16 code, void *sender, void *ListenerInst, EventContext context)
{
    const char* names[3] = {
        "Ice",
        "Sand",
        "Rope"
    };
    const char* SpecularNames[3] = {
        "Ice-Specular",
        "Sand-Specular",
        "Rope-Specular"
    };
    const char* NormalNames[3] = {
        "Ice-Normal",
        "Sand-Normal",
        "Rope-Normal"
    };
    static i8 choice = 2;

    // Сохраните старое имя.
    MString OldName = names[choice];
    MString OldSpecName = SpecularNames[choice];
    MString OldNormalName = NormalNames[choice];

    choice++;
    choice %= 3;

    // Создайте новую диффузную текстуру.
    if (State->TestGeometry) {
        State->TestGeometry->material->DiffuseMap.texture = TextureSystem::Instance()->Acquire(names[choice], true);
        if (!State->TestGeometry->material->DiffuseMap.texture) {
            MWARN("Event::OnDebugEvent нет диффузой текстуры! Используется значение по умолчанию");
            State->TestGeometry->material->DiffuseMap.texture = TextureSystem::Instance()->GetDefaultTexture();
        }

        // Освободите старую диффузную текстуру.
        TextureSystem::Instance()->Release(OldName.c_str());

        State->TestGeometry->material->SpecularMap.texture = TextureSystem::Instance()->Acquire(SpecularNames[choice], true);
        if (!State->TestGeometry->material->SpecularMap.texture) {
            MWARN("Event::OnDebugEvent нет зеркальной текстуры! Используется значение по умолчанию");
            State->TestGeometry->material->SpecularMap.texture = TextureSystem::Instance()->GetDefaultSpecularTexture();
        }

        // Освободите старую диффузную текстуру.
        TextureSystem::Instance()->Release(OldSpecName.c_str());

        State->TestGeometry->material->NormalMap.texture = TextureSystem::Instance()->Acquire(NormalNames[choice], true);
        if (!State->TestGeometry->material->NormalMap.texture) {
            MWARN("Event::OnDebugEvent нет текстуры нормалей! Используется значение по умолчанию");
            State->TestGeometry->material->NormalMap.texture = TextureSystem::Instance()->GetDefaultNormalTexture();
        }

        // Освободите старую диффузную текстуру.
        TextureSystem::Instance()->Release(OldNormalName.c_str());
    }

    return true;
}
//TODO: временно
