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
#include "systems/job_systems.hpp"

// ЗАДАЧА: временно
#include "math/geometry_utils.hpp"
#include "math/transform.hpp"
// ЗАДАЧА: временно

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

    GameInst->state = MMemory::Allocate(GameInst->StateMemoryRequirement, Memory::Application);
    GameInst->application->State = MMemory::TAllocate<ApplicationState>(Memory::Application);
    State = GameInst->application->State;
    State->GameInst = GameInst;

    u64 SystemsAllocatorTotalSize = 64 * 1024 * 1024;  // 64 mb
    LinearAllocator::Instance().Initialize(SystemsAllocatorTotalSize);

    // Инициализируйте подсистемы.
    State->logger = new Logger();
    if (!State->logger->Initialize()) { // State->logger->Initialize();
        MERROR("Не удалось инициализировать систему ведения журнала; завершение работы.");
        return false;
    }

    Input::Instance()->Initialize();

    State->IsRunning = true;
    State->IsSuspended = false;

    // ЗАДАЧА: temp debug
    State->ModelsLoaded = false;
    // ЗАДАЧА: end temp debug

    
    if (!Event::Initialize()) {
        MERROR("Система событий не смогла инициализироваться. Приложение не может быть продолжено.");
        return false;
    }
    State->Events = Event::GetInstance();

    State->Events->Register(EVENT_CODE_APPLICATION_QUIT, nullptr, OnEvent);
    State->Events->Register(EVENT_CODE_KEY_PRESSED, nullptr, OnKey);
    State->Events->Register(EVENT_CODE_KEY_RELEASED, nullptr, OnKey);
    State->Events->Register(EVENT_CODE_RESIZED, nullptr, OnResized);
    //ЗАДАЧА: временно
    State->Events->Register(EVENT_CODE_DEBUG0, nullptr, OnDebugEvent);
    State->Events->Register(EVENT_CODE_DEBUG1, nullptr, OnDebugEvent);
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
    
    State->Render = new Renderer(State->Window, GameInst->AppConfig.name, ERendererType::VULKAN);
    // Запуск рендерера
    if (!State->Render) {
        MFATAL("Не удалось инициализировать средство визуализации. Прерывание приложения.");
        return false;
    }
    bool RendererMultithreaded = State->Render->IsMultithreaded();

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
    if (!JobSystem::Initialize(ThreadCount, JobThreadTypes)) {
        MFATAL("Не удалось инициализировать систему заданий. Отмена приложения.");
        return false;
    }
    State->JobSystemInst = JobSystem::Instance();

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
    auto* SkyboxShader = ShaderSystem::GetInstance()->GetShader(BUILTIN_SHADER_NAME_SKYBOX);
    TextureMap* maps[1] = {&State->sb.cubemap};
    if (!Renderer::ShaderAcquireInstanceResources(SkyboxShader, maps, State->sb.InstanceID)) {
        MFATAL("Невозможно получить ресурсы шейдера для текстуры скайбокса.");
        return false;
    }

    // Мировые сетки
    // Отменить все сетки.
    for (u32 i = 0; i < 10; ++i) {
        State->meshes[i].generation = INVALID::U8ID;
        State->UIMeshes[i].generation = INVALID::U8ID;
    }

    u8 MeshCount = 0;
    // Загрузите конфигурацию и загрузите из нее геометрию.
    auto& CubeMesh = State->meshes[MeshCount];
    CubeMesh = Mesh(1, new GeometryID*[1], Transform());
    GeometryConfig gConfig = GeometrySystemInst->GenerateCubeConfig(10.f, 10.f, 10.f, 1.f, 1.f, "test_cube", "test_material");
    CubeMesh.geometries[0] = GeometrySystemInst->Acquire(gConfig, true);

    MeshCount++;
    CubeMesh.generation = 0;
    // Очистите места для конфигурации геометрии.
    gConfig.Dispose();

    auto& CubeMesh2 = State->meshes[MeshCount];
    CubeMesh2 = Mesh(1, new GeometryID*[1], Transform(FVec3(10.f, 0.f, 1.f)));
    gConfig = GeometrySystemInst->GenerateCubeConfig(5.f, 5.f, 5.f, 1.f, 1.f, "test_cube2", "test_material");
    CubeMesh2.geometries[0] = GeometrySystemInst->Acquire(gConfig, true);
    CubeMesh2.transform.SetParent(&CubeMesh.transform);
    MeshCount++;
    CubeMesh2.generation = 0;
    // Очистите места для конфигурации геометрии.
    gConfig.Dispose();

    auto& CubeMesh3 = State->meshes[MeshCount];
    CubeMesh3 = Mesh(1, new GeometryID*[1], Transform(FVec3(5.f, 0.f, 1.f)));
    gConfig = GeometrySystemInst->GenerateCubeConfig(2.f, 2.f, 2.f, 1.f, 1.f, "test_cube3", "test_material");
    CubeMesh3.geometries[0] = GeometrySystemInst->Acquire(gConfig, true);
    CubeMesh3.transform.SetParent(&CubeMesh2.transform);
    MeshCount++;
    CubeMesh3.generation = 0;
    // Очистите места для конфигурации геометрии.
    gConfig.Dispose();
    // Тестовая модель загружается из файла
    State->CarMesh = &State->meshes[MeshCount];
    
    State->CarMesh->transform = Transform(FVec3(15.f, 0.f, 1.f));
    MeshCount++;

    State->SponzaMesh = &State->meshes[MeshCount];

    State->SponzaMesh->transform = Transform(FVec3(), Quaternion::Identity(), FVec3(0.05, 0.05, 0.05));
    MeshCount++;

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
    State->UIMeshes[0].GeometryCount = 1;
    State->UIMeshes[0].geometries = new GeometryID*[1];
    State->UIMeshes[0].geometries[0] = GeometrySystemInst->Acquire(UI_Config, true);
    State->UIMeshes[0].transform = Transform();
    State->UIMeshes[0].generation = 0;

    // Загрузите геометрию по умолчанию.
    // State->TestGeometry = GeometrySystemInst->GetDefault();
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

            // Выполните небольшой поворот на первой сетке.
            Quaternion rotation( FVec3(0.f, 1.f, 0.f), 0.5f * delta, false );
            State->meshes[0].transform.Rotate(rotation);

            // Выполняем аналогичное вращение для второй сетки, если она существует.
            // «Родительский» второй куб по отношению к первому.
            State->meshes[1].transform.Rotate(rotation);

            // Выполняем аналогичное вращение для второй сетки, если она существует.
            // «Родительский» второй куб по отношению к первому.
            State->meshes[2].transform.Rotate(rotation);

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
            u32 MeshCount = 0;
            Mesh* meshes[10];
            // ЗАДАЧА: массив гибкого размера
            for (u32 i = 0; i < 10; ++i) {
                if (State->meshes[i].generation != INVALID::U8ID) {
                    meshes[MeshCount] = &State->meshes[i];
                    MeshCount++;
                }
            }
            Mesh::PacketData WorldMeshData {MeshCount, meshes};
            // ЗАДАЧА: выполняет поиск в каждом кадре.
            if (!State->RenderViewSystemInst->BuildPacket(State->RenderViewSystemInst->Get("world_opaque"), &WorldMeshData, packet.views[1])) {
                MERROR("Не удалось построить пакет для представления «world_opaque».");
                return false;
            }

            // пользовательский интерфейс
            u32 UIMeshCount = 0;
            Mesh* UIMeshes[10]{};
            // ЗАДАЧА: массив гибкого размера
            for (u32 i = 0; i < 10; ++i) {
                if (State->UIMeshes[i].generation != INVALID::U8ID) {
                    UIMeshes[UIMeshCount] = &State->UIMeshes[i];
                    UIMeshCount++;
                }
            }
            Mesh::PacketData UIMeshData = {UIMeshCount, UIMeshes};
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
    State->Events->Unregister(EVENT_CODE_APPLICATION_QUIT, nullptr, OnEvent);
    State->Events->Unregister(EVENT_CODE_KEY_PRESSED, nullptr, OnKey);
    State->Events->Unregister(EVENT_CODE_KEY_RELEASED, nullptr, OnKey);
    State->Events->Unregister(EVENT_CODE_RESIZED, nullptr, OnResized);
    //ЗАДАЧА: временно
    State->Events->Unregister(EVENT_CODE_DEBUG0, nullptr, OnDebugEvent);
    //ЗАДАЧА: временно
    State->Events->Shutdown(); // ЗАДАЧА: при удалении указателя на систему событий происходит ошибка
    Input::Instance()->Sutdown();
    GeometrySystem::Instance()->Shutdown();
    MaterialSystem::Shutdown();
    TextureSystem::Instance()->Shutdown();

    // ЗАДАЧА: Временное
    // ЗАДАЧА: реализовать уничтожение скайбокса.
    Renderer::TextureMapReleaseResources(&State->sb.cubemap);
    // ЗАДАЧА: Конец временного

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
    if (code == EVENT_CODE_DEBUG0) {
        const char* names[3] = {
            "Ice",
            "Sand",
            "Rope"
        };
        static i8 choice = 2;

        // Сохраните старое имя.
        const char* OldName = names[choice];

        choice++;
        choice %= 3;

        auto MaterialSystemInst = MaterialSystem::Instance();
        // Просто замените материал на первой сетке, если она существует.
        auto& g = State->meshes[0].geometries[0];
        if (g) {
            // Приобретите новый материал.
            g->material = MaterialSystemInst->Acquire(names[choice]);

        if (!g->material) {
                MWARN("Application::OnDebugEvent: материал не найден! Использование материала по умолчанию.");
                g->material = MaterialSystemInst->GetDefaultMaterial();
            }

            // Освободите старый рассеянный материал.
            MaterialSystemInst->Release(OldName);
        }
        return true;
    } else if (code == EVENT_CODE_DEBUG1) {
        if (!State->ModelsLoaded) {
            MDEBUG("Загружаем модели...");
            State->ModelsLoaded = true;
            if (!State->CarMesh->LoadFromResource("falcon")) {
                MERROR("Не удалось загрузить Falcon Mesh!");
            }
            if (!State->SponzaMesh->LoadFromResource("sponza")) {
                MERROR("Не удалось загрузить Falcon Mesh!");
            }
        }
        return true;
    }

    return true;
}
//ЗАДАЧА: временно
