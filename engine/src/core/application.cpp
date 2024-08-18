#include "application.hpp"
//#include "memory/linear_allocator.hpp"
#include "game_types.hpp"
#include "renderer/renderer.hpp"
#include "systems/texture_system.hpp"
#include "systems/material_system.hpp"
#include "systems/geometry_system.hpp"
#include "systems/resource_system.hpp"
#include "systems/shader_system.hpp"
#include "systems/camera_system.hpp"
#include "systems/render_view_system.hpp"

// ЗАДАЧА: временно
#include "math/geometry_utils.hpp"
#include "math/transform.hpp"
// ЗАДАЧА: временно

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

    GameInst->state = MMemory::Allocate(GameInst->StateMemoryRequirement, Memory::Application);
    GameInst->application->State = MMemory::TAllocate<ApplicationState>(Memory::Application);
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
    //ЗАДАЧА: временно
    Event::GetInstance()->Register(EVENT_CODE_DEBUG0, nullptr, OnDebugEvent);
    //ЗАДАЧА: временно
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
    State->ResourceSystemInst = ResourceSystem::Instance();

    // Система шейдеров
    if(!ShaderSystem::Initialize(1024, 128, 31, 31)) {
        MFATAL("Не удалось инициализировать шейдерную систему. Прерывание приложения.");
        return false;
    }

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
    auto GeometrySystemInst = GeometrySystem::Instance();

    // Система камер
    if (!CameraSystem::Initialize(61)) {
        MFATAL("Не удалось инициализировать систему камер. Приложение не может быть продолжено.");
        return false;
    }
    
    // Система визуадизаций
    if (!RenderViewSystem::Initialize(251)) {
        MFATAL("Не удалось инициализировать систему визуализаций. Приложение не может быть продолжено.");
        return false;
    }
    State->RenderViewSystemInst = RenderViewSystem::Instance();

    RenderView::PassConfig SkyboxPasses[1] { "Renderpass.Builtin.Skybox" };
    RenderView::Config SkyboxConfig {
        "skybox",
        0, 0, 
        RenderView::KnownTypeSkybox,
        RenderView::ViewMatrixSourceSceneCamera,
        1, SkyboxPasses
    };
    if (!State->RenderViewSystemInst->Create(SkyboxConfig)) {
        MFATAL("Не удалось создать представление скайбокса. Отмена приложения.");
        return false;
    }
    
    RenderView::PassConfig passes[1] { "Renderpass.Builtin.World" };
    RenderView::Config OpaqueWorldConfig {
        "world_opaque", 
        0, 0, 
        RenderView::KnownTypeWorld, 
        RenderView::ViewMatrixSourceSceneCamera, 
        1, passes
    };
    if (!State->RenderViewSystemInst->Create(OpaqueWorldConfig)) {
        MFATAL("Не удалось создать представление мира. Отмена приложения.");
        return false;
    }
    RenderView::PassConfig UIPasses[1] { "Renderpass.Builtin.UI" };
    RenderView::Config UIViewConfig {
        "ui",
        0, 0,
        RenderView::KnownTypeUI,
        RenderView::ViewMatrixSourceUiCamera,
        1, UIPasses
    };
    if (!State->RenderViewSystemInst->Create(UIViewConfig)) {
        MFATAL("Не удалось создать представление пользовательского интерфейса. Отмена приложения.");
        return false;
    }
    
    // ЗАДАЧА: временно
    // Skybox
    auto* CubeMap = &State->sb.cubemap;
    CubeMap->FilterMagnify = CubeMap->FilterMinify = TextureFilter::ModeLinear;
    CubeMap->RepeatU = CubeMap->RepeatV = CubeMap->RepeatW = TextureRepeat::ClampToEdge;
    CubeMap->use = TextureUse::Cubemap;
    if (!Renderer::TextureMapAcquireResources(CubeMap)) {
        MFATAL("Невозможно получить ресурсы для текстуры кубической карты.");
        return false;
    }
    CubeMap->texture = TextureSystem::Instance()->AcquireCube("skybox", true);
    auto SkyboxCubeConfig = GeometrySystemInst->GenerateCubeConfig(10.0f, 10.0f, 10.0f, 1.0f, 1.0f, "skybox_cube", nullptr);
    // Удалите название материала.
    SkyboxCubeConfig.MaterialName[0] = '\0';
    State->sb.g = GeometrySystemInst->Acquire(SkyboxCubeConfig, true);
    State->sb.RenderFrameNumber = INVALID::U64ID;
    Shader* SkyboxShader = ShaderSystem::GetInstance()->GetShader(BUILTIN_SHADER_NAME_SKYBOX);
    TextureMap* maps[1] = {&State->sb.cubemap};
    if (!Renderer::ShaderAcquireInstanceResources(SkyboxShader, maps, State->sb.InstanceID)) {
        MFATAL("Невозможно получить ресурсы шейдера для текстуры скайбокса.");
        return false;
    }

    // Мировые сетки
    // Загрузите конфигурацию и загрузите из нее геометрию.
    Mesh& CubeMesh = State->meshes[State->MeshCount];
    CubeMesh = Mesh(1, new GeometryID*[1], Transform());
    GeometryConfig gConfig = GeometrySystemInst->GenerateCubeConfig(10.f, 10.f, 10.f, 1.f, 1.f, "test_cube", "test_material");
    CubeMesh.geometries[0] = GeometrySystemInst->Acquire(gConfig, true);

    State->MeshCount++;
    // Очистите места для конфигурации геометрии.
    gConfig.Dispose();

    Mesh& CubeMesh2 = State->meshes[State->MeshCount];
    CubeMesh2 = Mesh(1, new GeometryID*[1], Transform(FVec3(10.f, 0.f, 1.f)));
    gConfig = GeometrySystemInst->GenerateCubeConfig(5.f, 5.f, 5.f, 1.f, 1.f, "test_cube2", "test_material");
    CubeMesh2.geometries[0] = GeometrySystemInst->Acquire(gConfig, true);
    CubeMesh2.transform.SetParent(&CubeMesh.transform);
    State->MeshCount++;
    // Очистите места для конфигурации геометрии.
    gConfig.Dispose();

    Mesh& CubeMesh3 = State->meshes[State->MeshCount];
    CubeMesh3 = Mesh(1, new GeometryID*[1], Transform(FVec3(5.f, 0.f, 1.f)));
    gConfig = GeometrySystemInst->GenerateCubeConfig(2.f, 2.f, 2.f, 1.f, 1.f, "test_cube3", "test_material");
    CubeMesh3.geometries[0] = GeometrySystemInst->Acquire(gConfig, true);
    CubeMesh3.transform.SetParent(&CubeMesh2.transform);
    State->MeshCount++;
    // Очистите места для конфигурации геометрии.
    gConfig.Dispose();
    // Тестовая модель загружается из файла
    Mesh& CarMesh = State->meshes[State->MeshCount];
    Resource CarMeshResource;
    if (!State->ResourceSystemInst->Load("falcon", ResourceType::Mesh, nullptr, CarMeshResource)) {
        MERROR("Не удалось загрузить тестовую модель машины!");
    } else {
        GeometryConfig* configs = reinterpret_cast<GeometryConfig*>(CarMeshResource.data);
        CarMesh.GeometryCount = CarMeshResource.DataSize;
        CarMesh.geometries = new GeometryID*[CarMesh.GeometryCount];
        for (u64 i = 0; i < CarMesh.GeometryCount; i++) {
            CarMesh.geometries[i] = GeometrySystemInst->Acquire(configs[i], true);
        }
        CarMesh.transform = Transform(FVec3(15.f, 0.f, 1.f));
        State->ResourceSystemInst->Unload(CarMeshResource);
        State->MeshCount++;
    }

    Mesh& SponzaMesh = State->meshes[State->MeshCount];
    Resource SponzaMeshResource;
    if (!State->ResourceSystemInst->Load("sponza", ResourceType::Mesh, nullptr, SponzaMeshResource)) {
        MERROR("Не удалось загрузить тестовую модель машины!");
    } else {
        GeometryConfig* configs = reinterpret_cast<GeometryConfig*>(SponzaMeshResource.data);
        SponzaMesh.GeometryCount = SponzaMeshResource.DataSize;
        SponzaMesh.geometries = new GeometryID*[SponzaMesh.GeometryCount];
        for (u64 i = 0; i < SponzaMesh.GeometryCount; i++) {
            SponzaMesh.geometries[i] = GeometrySystemInst->Acquire(configs[i], true);
        }
        SponzaMesh.transform = Transform(FVec3(), Quaternion::Identity(), FVec3(0.05, 0.05, 0.05));
        State->ResourceSystemInst->Unload(SponzaMeshResource);
        State->MeshCount++;
    }

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
    State->UIMeshCount = 1;
    State->UIMeshes[0].GeometryCount = 1;
    State->UIMeshes[0].geometries = new GeometryID*[1];
    State->UIMeshes[0].geometries[0] = GeometrySystemInst->Acquire(UI_Config, true);

    // Загрузите геометрию по умолчанию.
    // AppState->TestGeometry = GeometrySystemInst->GetDefault();
    // ЗАДАЧА: временно 

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

            if (State->MeshCount > 0) {
                // Выполните небольшой поворот на первой сетке.
                Quaternion rotation( FVec3(0.f, 1.f, 0.f), 0.5f * delta, false );
                State->meshes[0].transform.Rotate(rotation);

                // Выполняем аналогичное вращение для второй сетки, если она существует.
                if (State->MeshCount > 1) {
                    // «Родительский» второй куб по отношению к первому.
                    State->meshes[1].transform.Rotate(rotation);
                }

                // Выполняем аналогичное вращение для второй сетки, если она существует.
                if (State->MeshCount > 2) {
                    // «Родительский» второй куб по отношению к первому.
                    State->meshes[2].transform.Rotate(rotation);
                }
            }

            RenderView::Packet views[3]{};
            RenderPacket packet {
                delta, 
                //ЗАДАЧА: Прочтите конфигурацию фрейма.
                3, views
            };

            // Скайбокс
            SkyboxPacketData SkyboxData { &State->sb };
            if (!State->RenderViewSystemInst->BuildPacket(State->RenderViewSystemInst->Get("skybox"), &SkyboxData, packet.views[0])) {
                MERROR("Не удалось построить пакет для представления «skybox».");
                return false;
            }

            // Мир 
            Mesh::PacketData WorldMeshData {State->MeshCount, State->meshes};
            // ЗАДАЧА: выполняет поиск в каждом кадре.
            if (!State->RenderViewSystemInst->BuildPacket(State->RenderViewSystemInst->Get("world_opaque"), &WorldMeshData, packet.views[1])) {
                MERROR("Не удалось построить пакет для представления «world_opaque».");
                return false;
            }

            // пользовательский интерфейс
            Mesh::PacketData UIMeshData = {State->UIMeshCount, State->UIMeshes};
            if (!State->RenderViewSystemInst->BuildPacket(State->RenderViewSystemInst->Get("ui"), &UIMeshData, packet.views[2])) {
                MERROR("Не удалось построить пакет для представления «ui».");
                return false;
            }
            // ЗАДАЧА: временно

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
    //ЗАДАЧА: временно
    Event::GetInstance()->Unregister(EVENT_CODE_DEBUG0, nullptr, OnDebugEvent);
    //ЗАДАЧА: временно
    Event::GetInstance()->Shutdown(); // ЗАДАЧА: при удалении указателя на систему событий происходит ошибка
    Input::Instance()->Sutdown();
    GeometrySystem::Instance()->Shutdown();
    MaterialSystem::Shutdown();
    TextureSystem::Instance()->Shutdown();

    // ЗАДАЧА: Временное
    // ЗАДАЧА: реализовать уничтожение скайбокса.
    Renderer::TextureMapReleaseResources(&State->sb.cubemap);
    // ЗАДАЧА: Конец временного

    ShaderSystem::Shutdown();
    State->ResourceSystemInst->Shutdown();
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
                return true;
            }
        }
    }
    // Событие намеренно не обрабатывается, чтобы другие слушатели могли его получить.
    return false;
}

// ЗАДАЧА: временно
bool Application::OnDebugEvent(u16 code, void *sender, void *ListenerInst, EventContext context)
{
    const char* names[3] = {
        "Ice",
        "Sand",
        "Rope"
    };
    static i8 choice = 2;

    // Сохраните старое имя.
    MString OldName = names[choice];

    choice++;
    choice %= 3;

    // Просто замените материал на первой сетке, если она существует.
    GeometryID* g = State->meshes[0].geometries[0];
    if (g) {
        // Приобретите новый материал.
        g->material = MaterialSystem::Instance()->Acquire(names[choice]);

    if (!g->material) {
            MWARN("Application::OnDebugEvent: материал не найден! Использование материала по умолчанию.");
            g->material = MaterialSystem::Instance()->GetDefaultMaterial();
        }

        // Освободите старый рассеянный материал.
        MaterialSystem::Instance()->Release(OldName.c_str());
    }

    return true;
}
//ЗАДАЧА: временно
