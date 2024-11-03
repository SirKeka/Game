#include "game.hpp"

#include <core/logger.hpp>

#include <systems/camera_system.hpp>
#include <renderer/renderer.hpp>
#include <new>

// ЗАДАЧА: временный код
#include <core/identifier.hpp>
#include <systems/geometry_system.hpp>
#include <systems/render_view_system.hpp>
#include <renderer/views/render_view_pick.hpp>
#include "debug_console.hpp"
// ЗАДАЧА: конец временного кода

bool Game::OnEvent(u16 code, void *sender, void *ListenerInst, EventContext context)
{
    auto GameInst = reinterpret_cast<Game*>(ListenerInst);
    auto state = reinterpret_cast<Game::State*>(GameInst->state);
    switch (code) {
        case EVENT_CODE_OBJECT_HOVER_ID_CHANGED: {
            state->HoveredObjectID = context.data.u32[0];
            return true;
        }
    }

    return false;
}

// ЗАДАЧА: временно
bool Game::OnDebugEvent(u16 code, void *sender, void *ListenerInst, EventContext context)
{
    auto GameInst = reinterpret_cast<Game*>(ListenerInst);
    auto state = reinterpret_cast<Game::State*>(GameInst->state);
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
        auto& g = state->meshes[0].geometries[0];
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
        if (!state->ModelsLoaded) {
            MDEBUG("Загружаем модели...");
            state->ModelsLoaded = true;
            if (!state->CarMesh->LoadFromResource("falcon")) {
                MERROR("Не удалось загрузить Falcon Mesh!");
            }
            if (!state->SponzaMesh->LoadFromResource("sponza")) {
                MERROR("Не удалось загрузить Falcon Mesh!");
            }
        }
        return true;
    }

    return true;
}
//ЗАДАЧА: временно

bool GameOnKey(u16 code, void *sender, void *ListenerInst, EventContext context)
{
    // if (code == EVENT_CODE_KEY_PRESSED) {
    //     u16 KeyCode = context.data.u16[0];
    //     if (KeyCode == Keys::A) {
    //         // Пример проверки клавиши
    //         MDEBUG("Явно — клавиша A нажата!");
    //     } else {
    //         MDEBUG("'%c' нажатие клавиши в окне.", KeyCode);
    //     }
    // } else if (code == EVENT_CODE_KEY_RELEASED) {
    //     u16 KeyCode = context.data.u16[0];
    //     if (KeyCode == Keys::B) {
    //         // Пример проверки клавиши
    //         MDEBUG("Явно — клавиша B отущена!");
    //     } else {
    //         MDEBUG("'%c' клавиша отпущена в окне.", KeyCode);
    //     }
    // }

    return false;
}

Game::~Game()
{
    Shutdown();
}

bool Game::Boot()
{
    MINFO("Загрузка испытательной сборки...");

    DebugConsole::Create();

    // Установка распределителя кадра
    FrameAllocator.Initialize(MEBIBYTES(64), nullptr);

    AppConfig.FontConfig.AutoRelease = false;
    AppConfig.FontConfig.DefaultBitmapFontCount = 1;

    // BitmapFontConfig BmpFontConfig { "Metrika 21px", 21, "Metrika" };
    AppConfig.FontConfig.BitmapFontConfigs = new BitmapFontConfig("Metrika 21px", 21, "Metrika");

    // SystemFontConfig SysFontConfig { "Noto Sans", 20, "NotoSansCJK" };
    AppConfig.FontConfig.DefaultSystemFontCount = 1;
    AppConfig.FontConfig.SystemFontConfigs = new SystemFontConfig("Metrika", 20, "Metrika");

    AppConfig.FontConfig.MaxBitmapFontCount = AppConfig.FontConfig.MaxSystemFontCount = 101;

    // Настроить представления рендеринга. ЗАДАЧА: прочитать из файла?
    if (!ConfigureRenderViews()) {
        MERROR("Не удалось настроить представления рендерера. Прерывание приложения.");
        return false;
    }

    // Раскладки клавиатуры
    SetupKeymaps();
    // Консольные команды
    SetupCommands();

    return true;
}

bool Game::Initialize()
{
    MDEBUG("Game::Initialize вызван!");

    DebugConsole::Load();

    auto state = reinterpret_cast<State*>(this->state);

    // ЗАДАЧА: временная загрузка/подготовка вещей
    state->ModelsLoaded = false;

    if (!state->TestText.Create(TextType::Bitmap, "Metrika 21px", 21, "Какой-то тестовый текст 123,\n\tyo!")) {
        MERROR("Не удалось загрузить базовый растровый текст пользовательского интерфейса.");
        return false;
    }
    // Переместить отладочный текст в новую нижнюю часть экрана.
    state->TestText.SetPosition(FVec3(20.F, state->height - 75.F, 0.F)); 

    if(!state->TestSysText.Create(TextType::System, "Metrika", 31, "Какой-то тестовый текст 123, \n\tyo!\n\n\t")) { // Noto Sans CJK JP こんにちは 한
        MERROR("Не удалось загрузить базовый системный текст пользовательского интерфейса.");
        return false;
    }
    state->TestSysText.SetPosition(FVec3(500.F, 250.F, 0.F));

    state->sb.Create("skybox_cube");

    // Мировые сетки
    // Отменить все сетки.
    for (u32 i = 0; i < 10; ++i) {
        state->meshes[i].generation = INVALID::U8ID;
        state->UiMeshes[i].generation = INVALID::U8ID;
    }

    u8 MeshCount = 0;
    // Загрузите конфигурацию и загрузите из нее геометрию.
    auto& CubeMesh = state->meshes[MeshCount];
    CubeMesh = Mesh(1, new GeometryID*[1], Transform());
    auto gConfig = GeometrySystem::GenerateCubeConfig(10.F, 10.F, 10.F, 1.F, 1.F, "test_cube", "test_material");
    CubeMesh.geometries[0] = GeometrySystem::Acquire(gConfig, true);

    MeshCount++;
    CubeMesh.generation = 0;
    CubeMesh.UniqueID = Identifier::AquireNewID(&CubeMesh);
    // Очистите места для конфигурации геометрии.
    gConfig.Dispose();

    auto& CubeMesh2 = state->meshes[MeshCount];
    CubeMesh2 = Mesh(1, new GeometryID*[1], Transform(FVec3(10.F, 0.F, 1.F)));
    gConfig = GeometrySystem::GenerateCubeConfig(5.F, 5.F, 5.F, 1.F, 1.F, "test_cube2", "test_material");
    CubeMesh2.geometries[0] = GeometrySystem::Acquire(gConfig, true);
    CubeMesh2.transform.SetParent(&CubeMesh.transform);
    MeshCount++;
    CubeMesh2.generation = 0;
    CubeMesh2.UniqueID = Identifier::AquireNewID(&CubeMesh2);
    // Очистите места для конфигурации геометрии.
    gConfig.Dispose();

    auto& CubeMesh3 = state->meshes[MeshCount];
    CubeMesh3 = Mesh(1, new GeometryID*[1], Transform(FVec3(5.f, 0.f, 1.f)));
    gConfig = GeometrySystem::GenerateCubeConfig(2.f, 2.f, 2.f, 1.f, 1.f, "test_cube3", "test_material");
    CubeMesh3.geometries[0] = GeometrySystem::Acquire(gConfig, true);
    CubeMesh3.transform.SetParent(&CubeMesh2.transform);
    MeshCount++;
    CubeMesh3.generation = 0;
    CubeMesh3.UniqueID = Identifier::AquireNewID(&CubeMesh3);
    // Очистите места для конфигурации геометрии.
    gConfig.Dispose();
    // Тестовая модель загружается из файла
    state->CarMesh = &state->meshes[MeshCount];
    state->CarMesh->UniqueID = Identifier::AquireNewID(&state->CarMesh);
    state->CarMesh->transform = Transform(FVec3(15.F, 0.F, 1.F));
    MeshCount++;

    state->SponzaMesh = &state->meshes[MeshCount];
    state->SponzaMesh->UniqueID = Identifier::AquireNewID(&state->SponzaMesh);
    state->SponzaMesh->transform = Transform(FVec3(), Quaternion::Identity(), FVec3(0.05F, 0.05F, 0.05F));
    MeshCount++;

    const f32 w = 128.F;
    const f32 h = 49.F;
    Vertex2D uiverts [4];
    uiverts[0].position.x = 0.F;  // 0    3
    uiverts[0].position.y = 0.F;  //
    uiverts[0].texcoord.x = 0.F;  //
    uiverts[0].texcoord.y = 0.F;  // 2    1

    uiverts[1].position.y = h;
    uiverts[1].position.x = w;
    uiverts[1].texcoord.x = 1.F;
    uiverts[1].texcoord.y = 1.F;

    uiverts[2].position.x = 0.F;
    uiverts[2].position.y = h;
    uiverts[2].texcoord.x = 0.F;
    uiverts[2].texcoord.y = 1.F;

    uiverts[3].position.x = w;
    uiverts[3].position.y = 0.0;
    uiverts[3].texcoord.x = 1.F;
    uiverts[3].texcoord.y = 0.F;

    // Индексы - против часовой стрелки
    u32 uiindices[6] = {2, 1, 0, 3, 0, 1};
    GeometryConfig UI_Config;
    UI_Config.VertexSize = sizeof(Vertex2D);
    UI_Config.VertexCount = 4;
    UI_Config.vertices = uiverts;
    UI_Config.IndexSize = sizeof(u32);
    UI_Config.IndexCount = 6;
    UI_Config.indices = uiindices;
    UI_Config.CopyNames("test_ui_geometry", "test_ui_material");

    // Получите геометрию пользовательского интерфейса из конфигурации.
    state->UiMeshes[0].UniqueID = Identifier::AquireNewID(&state->UiMeshes[0]);
    state->UiMeshes[0].GeometryCount = 1;
    state->UiMeshes[0].geometries = new GeometryID*[1];
    state->UiMeshes[0].geometries[0] = GeometrySystem::Acquire(UI_Config, true);
    state->UiMeshes[0].transform = Transform();
    state->UiMeshes[0].generation = 0;

    state->UiMeshes[0].transform.Translate(FVec3(650, 5, 0));
    
    state->WorldCamera = CameraSystem::GetDefault();
    state->WorldCamera->SetPosition(FVec3(10.5F, 5.F, 9.5F));

    //ЗАДАЧА: временно
    Event::Register(EVENT_CODE_DEBUG0, this, OnDebugEvent);
    Event::Register(EVENT_CODE_DEBUG1, this, OnDebugEvent);
    Event::Register(EVENT_CODE_OBJECT_HOVER_ID_CHANGED, this, OnEvent);
    //ЗАДАЧА: временно

    Event::Register(EVENT_CODE_KEY_PRESSED,  this, GameOnKey);
    Event::Register(EVENT_CODE_KEY_RELEASED, this, GameOnKey);

    state->UpdateClock.Zero();
    state->RenderClock.Zero();
    
    return true;
}

bool Game::Update(f32 DeltaTime)
{
    // Убедитесь, что это очищено, чтобы избежать утечки памяти.
    // ЗАДАЧА: Нужна версия этого, которая использует распределитель кадров.
    if (WorldGeometries) {
        WorldGeometries.Clear();
    }

    FrameAllocator.FreeAll();

    auto state = reinterpret_cast<State*>(this->state);

    state->UpdateClock.Start();
    state->DeltaTime = DeltaTime;

    // Отслеживайте различия в распределении.
    state->PrevAllocCount = state->AllocCount;
    state->AllocCount = MMemory::GetMemoryAllocCount();

    // Отслеживайте различия в распределении.
    // state->PrevAllocCount = state->AllocCount;
    // state->AllocCount = GetMemoryAllocCount();

    // Выполните небольшой поворот на первой сетке.
    Quaternion rotation( FVec3(0.F, 1.F, 0.F), 0.5F * DeltaTime, false );
    state->meshes[0].transform.Rotate(rotation);

    // Выполняем аналогичное вращение для второй сетки, если она существует.
    // «Родительский» второй куб по отношению к первому.
    state->meshes[1].transform.Rotate(rotation);

    // Выполняем аналогичное вращение для второй сетки, если она существует.
    // «Родительский» второй куб по отношению к первому.
    state->meshes[2].transform.Rotate(rotation);

    // Обновляем растровый текст с позицией камеры. ПРИМЕЧАНИЕ: пока используем камеру по умолчанию.
    auto WorldCamera = CameraSystem::GetDefault();
    const auto& pos = WorldCamera->GetPosition();
    const auto& rot = WorldCamera->GetRotationEuler();

    // Также прикрепите текущее состояние мыши.
    bool LeftDown = Input::IsButtonDown(Buttons::Left);
    bool RightDown = Input::IsButtonDown(Buttons::Right);
    i16 MouseX, MouseY;
    Input::GetMousePosition(MouseX, MouseY);

    // Преобразовать в NDC
    f32 mouseXNdc = Math::RangeConvertF32((f32)MouseX, 0.F, (f32)state->width,  -1.F, 1.F);
    f32 MouseYNdc = Math::RangeConvertF32((f32)MouseY, 0.F, (f32)state->height, -1.F, 1.F);

    f64 fps, FrameTime;
    Metrics::Frame(fps, FrameTime);

    // Обновите усеченную пирамиду
    const auto& forward = state->WorldCamera->Forward();
    const auto& right = state->WorldCamera->Right();
    const auto& up = state->WorldCamera->Up();
    // ЗАДАЧА: получите поле зрения камеры, аспект и т. д.
    state->CameraFrustrum.Create(state->WorldCamera->GetPosition(), forward, right, up, (f32)state->width / state->height, Math::DegToRad(45.F), 0.1F, 1000.F);

    // ПРИМЕЧАНИЕ: начните с разумного значения по умолчанию, чтобы избежать слишком большого количества перераспределений.
    WorldGeometries.Reserve(512);
    u32 DrawCount = 0;
    for (u32 i = 0; i < 10; ++i) {
        auto& m = state->meshes[i];
        if (m.generation != INVALID::U8ID) {
            auto model = m.transform.GetWorld();

            for (u32 j = 0; j < m.GeometryCount; ++j) {
                auto g = m.geometries[j];

                // Расчет ограничивающей сферы.
                // {
                //     // Переместите/масштабируйте экстенты.
                //     auto ExtentsMin = g->extents.MinSize * model;
                //     auto ExtentsMax = g->extents.MaxSize * model;

                //     f32 min = MMIN(MMIN(ExtentsMin.x, ExtentsMin.y), ExtentsMin.z);
                //     f32 max = MMAX(MMAX(ExtentsMax.x, ExtentsMax.y), ExtentsMax.z);
                //     f32 diff = Math::abs(max - min);
                //     f32 radius = diff * 0.5f;

                //     // Переместите/масштабируйте центр.
                //     auto center = g->center * model;

                //     if (state->CameraFrustrum.IntersectsSphere(center, radius)) {
                //         // Добавьте его в список для рендеринга.
                //         GeometryRenderData data = {};
                //         data.model = model;
                //         data.gid = g;
                //         data.UniqueID = m.UniqueID;
                //         WorldGeometries.PushBack(data);

                //         DrawCount++;
                //     }
                // }

                // Расчет AABB
                {
                    // Переместите/масштабируйте экстенты.
                    // auto ExtentsMin = g->extents.MinSize * model;
                    auto ExtentsMax = g->extents.MaxSize * model;
                    
                    // Переместить/масштабировать центр.
                    auto center = g->center * model;
                    FVec3 HalfExtents{
                        Math::abs(ExtentsMax.x - center.x),
                        Math::abs(ExtentsMax.y - center.y),
                        Math::abs(ExtentsMax.z - center.z),
                    };

                    if (state->CameraFrustrum.IntersectsAABB(center, HalfExtents)) {
                        // Добавьте его в список для рендеринга.
                        GeometryRenderData data = {};
                        data.model = model;
                        data.gid = g;
                        data.UniqueID = m.UniqueID;
                        WorldGeometries.PushBack(data);

                        DrawCount++;
                    }
                }
            }
        }
    }

    const char* VsyncText = Renderer::FlagEnabled(RendererConfigFlagBits::VsyncEnabledBit) ? "Вкл" : "Выкл";
    char TextBuffer[2048]{};
    MString::Format(
        TextBuffer,
        "\
        FPS: %5.1f(%4.1fмс) Позиция=[%7.3F, %7.3F, %7.3F] Вращение=[%7.3F, %7.3F, %7.3F]\n\
        Upd: %8.3fмкс, Rend: %8.3fмкс Мышь: X=%-5d Y=%-5d   L=%s R=%s   NDC: X=%.6f, Y=%.6f\n\
        Vsync: %s Draw: %-5u Hovered: %s%u",
        fps,
        FrameTime,
        pos.x, pos.y, pos.z,
        Math::RadToDeg(rot.x), Math::RadToDeg(rot.y), Math::RadToDeg(rot.z),
        state->LastUpdateElapsed * M_SEC_TO_US_MULTIPLIER,
        state->RenderClock.elapsed * M_SEC_TO_US_MULTIPLIER,
        MouseX, MouseY,
        LeftDown ? "Y" : "N",
        RightDown ? "Y" : "N",
        mouseXNdc,
        MouseYNdc,
        VsyncText,
        DrawCount,
        state->HoveredObjectID == INVALID::ID ? "none" : "",
        state->HoveredObjectID == INVALID::ID ? 0 : state->HoveredObjectID
    );
    state->TestText.SetText(TextBuffer);

    DebugConsole::Update();

    state->UpdateClock.Update();
    state->LastUpdateElapsed = state->UpdateClock.elapsed;

    return true;
}

bool Game::Render(RenderPacket& packet, f32 DeltaTime) 
{
    auto state = reinterpret_cast<State*>(this->state);

    state->RenderClock.Start();
    // ЗАДАЧА: временный код

    // ЗАДАЧА: Чтение из конфигурации кадра
    packet.ViewCount = 4;
    packet.views = reinterpret_cast<RenderView::Packet*>(FrameAllocator.Allocate(sizeof(RenderView::Packet) * packet.ViewCount));

    // Скайбокс
    SkyboxPacketData* SkyboxData = reinterpret_cast<SkyboxPacketData*>(FrameAllocator.Allocate(sizeof(SkyboxPacketData)));
    SkyboxData->sb = &state->sb;
    if (!RenderViewSystem::BuildPacket(RenderViewSystem::Get("skybox"), FrameAllocator, SkyboxData, packet.views[0])) {
        MERROR("Не удалось построить пакет для представления «skybox».");
        return false;
    }
    
    // Мир 
    // ЗАДАЧА: выполняет поиск в каждом кадре.
    if (!RenderViewSystem::BuildPacket(RenderViewSystem::Get("world"), FrameAllocator, &WorldGeometries, packet.views[1])) {
        MERROR("Не удалось построить пакет для представления «world_opaque».");
        return false;
    }

    // Пользовательский интерфейс
    UiPacketData UiPacket{};
    u32 UIMeshCount = 0;
    u32 MaxUiMeshes = 10;
    Mesh** UIMeshes = reinterpret_cast<Mesh**>(FrameAllocator.Allocate(sizeof(Mesh*) * MaxUiMeshes));
    // ЗАДАЧА: массив гибкого размера
    for (u32 i = 0; i < 10; ++i) {
        if (state->UiMeshes[i].generation != INVALID::U8ID) {
            UIMeshes[UIMeshCount] = &state->UiMeshes[i];
            UIMeshCount++;
        }
    }
    UiPacket.MeshData.MeshCount = UIMeshCount;
    UiPacket.MeshData.meshes = UIMeshes;
    UiPacket.TextCount = 2;
    Text* DebugConsoleText = DebugConsole::GetText();
    bool RenderDebugConsole = DebugConsoleText && DebugConsole::Visible();
    if (RenderDebugConsole) {
        UiPacket.TextCount += 2;
    }
    Text** texts = reinterpret_cast<Text**>(FrameAllocator.Allocate(sizeof(Text*) * UiPacket.TextCount));
    texts[0] = &state->TestText;
    texts[1] = &state->TestSysText;
    if (RenderDebugConsole) {
        texts[2] = DebugConsoleText;
        texts[3] = DebugConsole::GetEntryText();
    }
    UiPacket.texts = texts;
    if (!RenderViewSystem::BuildPacket(RenderViewSystem::Get("ui"), FrameAllocator, &UiPacket, packet.views[2])) {
        MERROR("Не удалось построить пакет для представления «ui».");
        return false;
    }

    RenderViewPick::PacketData PickPacket;
    PickPacket.UiMeshData = UiPacket.MeshData;
    PickPacket.WorldMeshData = WorldGeometries;
    PickPacket.texts = UiPacket.texts;
    PickPacket.TextCount = UiPacket.TextCount;
    PickPacket.UiGeometryCount = 0;

    if (!RenderViewSystem::BuildPacket(RenderViewSystem::Get("pick"), FrameAllocator, &PickPacket, packet.views[3])) {
        MERROR("Не удалось построить пакет для представления «pick».");
        return false;
    }

    // ЗАДАЧА: конец временного кода

    state->RenderClock.Update();
    
    return true;
}

void Game::OnResize(u32 width, u32 height) 
{
    auto state = reinterpret_cast<State*>(this->state);

    state->width = width;
    state->height = height;

    // ЗАДАЧА: временный блок кода
    // Переместить отладочный текст в новую нижнюю часть экрана.
    state->TestText.SetPosition(FVec3(20, state->height - 75, 0));
    // ЗАДАЧА: конец временного блока кода
}

void Game::Shutdown()
{
    auto state = reinterpret_cast<State*>(this->state);

    // ЗАДАЧА: Временный блок кода
    state->sb.Destroy();

    // Уничтожение текста пользовательского интерфейса
    // state->TestText.~Text();
    // state->TestSysText.~Text();

    Event::Unregister(EVENT_CODE_DEBUG0, this, OnDebugEvent);
    Event::Unregister(EVENT_CODE_DEBUG1, this, OnDebugEvent);
    Event::Unregister(EVENT_CODE_OBJECT_HOVER_ID_CHANGED, this, OnEvent);
    // ЗАДАЧА: конец временного блока кода

    Event::Unregister(EVENT_CODE_KEY_PRESSED, this, GameOnKey);
    Event::Unregister(EVENT_CODE_KEY_RELEASED, this, GameOnKey);
}

bool Game::ConfigureRenderViews()
{
    AppConfig.RenderViews.Resize(4); // .Resize(4);

    DArray<RenderTargetAttachmentConfig> SkyboxTargetAttachment;
    SkyboxTargetAttachment.Resize(1);
    SkyboxTargetAttachment[0].type = RenderTargetAttachmentType::Colour;
    SkyboxTargetAttachment[0].source = RenderTargetAttachmentSource::Default;
    // SkyboxTargetAttachment[0].LoadOperation = RenderTargetAttachmentLoadOperation::DontCare;
    SkyboxTargetAttachment[0].StoreOperation = RenderTargetAttachmentStoreOperation::Store;
    // SkyboxTargetAttachment[0].PresentAfter = false;

    // Конфигурация подпрохода рендеринга
    DArray<RenderpassConfig> SkyboxPasses;
    SkyboxPasses.Resize(1);
    SkyboxPasses[0].name = "Renderpass.Bultin.Skybox";
    SkyboxPasses[0].depth = 1.F;
    //SkyboxPasses[0].stencil = 0;
    SkyboxPasses[0].RenderArea = FVec4(0.F, 0.F, 1280.F, 720.F);
    SkyboxPasses[0].ClearColour = FVec4(0.F, 0.F, 0.2F, 1.F);
    SkyboxPasses[0].ClearFlags = RenderpassClearFlag::ColourBuffer;
    SkyboxPasses[0].RenderTargetCount = Renderer::WindowAttachmentCountGet();
    SkyboxPasses[0].target.AttachmentCount = 1;
    SkyboxPasses[0].target.attachments = SkyboxTargetAttachment.MovePtr();

    // Skybox view
    AppConfig.RenderViews[0].name = "skybox";
    // AppConfig.RenderViews[0].CustomShaderName
    // AppConfig.RenderViews[0].width = 0;
    // AppConfig.RenderViews[0].height = 0;
    AppConfig.RenderViews[0].type = RenderView::KnownTypeSkybox;
    AppConfig.RenderViews[0].VMS = RenderView::ViewMatrixSourceSceneCamera;
    AppConfig.RenderViews[0].PassCount = 1;
    AppConfig.RenderViews[0].passes = SkyboxPasses.MovePtr();

    // World view
    DArray<RenderTargetAttachmentConfig> WorldTargetAttachment;
    WorldTargetAttachment.Resize(2);
    // Привязка цвета
    WorldTargetAttachment[0].type           = RenderTargetAttachmentType::Colour;
    WorldTargetAttachment[0].source         = RenderTargetAttachmentSource::Default;
    WorldTargetAttachment[0].LoadOperation  = RenderTargetAttachmentLoadOperation::Load;
    WorldTargetAttachment[0].StoreOperation = RenderTargetAttachmentStoreOperation::Store;
    // WorldTargetAttachment[0].PresentAfter = false;
    // Привязка глубины
    WorldTargetAttachment[1].type           = RenderTargetAttachmentType::Depth;
    WorldTargetAttachment[1].source         = RenderTargetAttachmentSource::Default;
    // WorldTargetAttachment[1].LoadOperation  = RenderTargetAttachmentLoadOperation::DontCare;
    WorldTargetAttachment[1].StoreOperation = RenderTargetAttachmentStoreOperation::Store;
    // WorldTargetAttachment[1].PresentAfter = false;

    // Конфигурация подпрохода рендеринга
    DArray<RenderpassConfig> WorldPasses;
    WorldPasses.Resize(1);
    WorldPasses[0].name = "Renderpass.Bultin.World";
    WorldPasses[0].depth = 1.F;
    // WorldPasses[0].stencil = 0;
    WorldPasses[0].RenderArea = FVec4(0.F, 0.F, 1280.F, 720.F); // Разрешение области рендеринга по умолчанию.
    WorldPasses[0].ClearColour = FVec4(0.F, 0.F, 0.2F, 1.F); 
    WorldPasses[0].ClearFlags = RenderpassClearFlag::DepthBuffer | RenderpassClearFlag::StencilBuffer;
    WorldPasses[0].RenderTargetCount = Renderer::WindowAttachmentCountGet();
    WorldPasses[0].target.AttachmentCount = 2;
    WorldPasses[0].target.attachments = WorldTargetAttachment.MovePtr();

    AppConfig.RenderViews[1].name = "world";
    // AppConfig.RenderViews[1].width = 0;
    // AppConfig.RenderViews[1].height = 0;
    AppConfig.RenderViews[1].type = RenderView::KnownTypeWorld;
    AppConfig.RenderViews[1].VMS = RenderView::ViewMatrixSourceSceneCamera;
    AppConfig.RenderViews[1].PassCount = 1;
    AppConfig.RenderViews[1].passes = WorldPasses.MovePtr();

    // UI view
    DArray<RenderTargetAttachmentConfig> UiTargetAttachment;
    UiTargetAttachment.Resize(1);
    UiTargetAttachment[0].type           = RenderTargetAttachmentType::Colour;
    UiTargetAttachment[0].source         = RenderTargetAttachmentSource::Default;
    UiTargetAttachment[0].LoadOperation  = RenderTargetAttachmentLoadOperation::Load;
    UiTargetAttachment[0].StoreOperation = RenderTargetAttachmentStoreOperation::Store;
    UiTargetAttachment[0].PresentAfter   = true;

    // Конфигурация подпрохода рендеринга
    DArray<RenderpassConfig> UIPasses;
    UIPasses.Resize(1);
    UIPasses[0].name = "Renderpass.Bultin.UI";
    UIPasses[0].depth = 1.F;
    UIPasses[0].stencil = 0;
    UIPasses[0].RenderArea = FVec4(0.F, 0.F, 1280.F, 720.F); // Разрешение области рендеринга по умолчанию.
    UIPasses[0].ClearColour = FVec4(0.F, 0.F, 0.2F, 1.F);
    UIPasses[0].ClearFlags = RenderpassClearFlag::None;
    UIPasses[0].RenderTargetCount = Renderer::WindowAttachmentCountGet();
    UIPasses[0].target.AttachmentCount = 1;
    UIPasses[0].target.attachments = UiTargetAttachment.MovePtr();

    AppConfig.RenderViews[2].name = "ui";
    // AppConfig.RenderViews[2].width = 0;
    // AppConfig.RenderViews[2].height = 0;
    AppConfig.RenderViews[2].type = RenderView::KnownTypeUI;
    AppConfig.RenderViews[2].VMS = RenderView::ViewMatrixSourceUiCamera;
    AppConfig.RenderViews[2].PassCount = 1;
    AppConfig.RenderViews[2].passes = UIPasses.MovePtr();

    // Pick view
    DArray<RenderTargetAttachmentConfig> WorldPickTargetAttachment;
    WorldPickTargetAttachment.Resize(2);
    // Привязка цвета
    WorldPickTargetAttachment[0].type = RenderTargetAttachmentType::Colour;
    WorldPickTargetAttachment[0].source = RenderTargetAttachmentSource::View; // Получить вложение из представления.
    // WorldPickTargetAttachment[0].LoadOperation = RenderTargetAttachmentLoadOperation::DontCare;
    WorldPickTargetAttachment[0].StoreOperation = RenderTargetAttachmentStoreOperation::Store;
    // WorldPickTargetAttachment[0].PresentAfter = false;
    // Привязка глубины
    WorldPickTargetAttachment[1].type = RenderTargetAttachmentType::Depth;
    WorldPickTargetAttachment[1].source = RenderTargetAttachmentSource::View; // Получить вложение из представления.
    // WorldPickTargetAttachment[1].LoadOperation = RenderTargetAttachmentLoadOperation::DontCare;
    WorldPickTargetAttachment[1].StoreOperation = RenderTargetAttachmentStoreOperation::Store;
    // WorldPickTargetAttachment[1].PresentAfter = false;

    DArray<RenderTargetAttachmentConfig> UiPickTargetAttachment;
    UiPickTargetAttachment.Resize(1);
    // Привязка цвета
    UiPickTargetAttachment[0].type = RenderTargetAttachmentType::Colour;
    UiPickTargetAttachment[0].source = RenderTargetAttachmentSource::View; // Получить вложение из представления.
    UiPickTargetAttachment[0].LoadOperation = RenderTargetAttachmentLoadOperation::Load;
    UiPickTargetAttachment[0].StoreOperation = RenderTargetAttachmentStoreOperation::Store; // Необходимо сохранить его, чтобы впоследствии можно было взять образец.
    // UiPickTargetAttachment.PresentAfter = false;

    // Конфигурация подпрохода рендеринга
    DArray<RenderpassConfig> PickPasses;
    PickPasses.Resize(2);
    PickPasses[0].name = "Renderpass.Builtin.WorldPick";
    PickPasses[0].depth = 1.F;
    // PickPasses[0].stencil = 0;
    PickPasses[0].RenderArea = FVec4(0.F, 0.F, 1280.F, 720.F);  // Разрешение области рендеринга по умолчанию.
    PickPasses[0].ClearColour = FVec4(1.F, 1.F, 1.F, 1.F);      // ХАК: очищаем до белого цвета для лучшей видимости// ЗАДАЧА: очищаем до черного цвета, так как 0 — недопустимый идентификатор.
    PickPasses[0].ClearFlags = RenderpassClearFlag::ColourBuffer | RenderpassClearFlag::DepthBuffer;
    PickPasses[0].RenderTargetCount = 1;
    PickPasses[0].target.AttachmentCount = 2;
    PickPasses[0].target.attachments = WorldPickTargetAttachment.MovePtr();

    PickPasses[1].name = "Renderpass.Bultin.UiPick";
    PickPasses[1].depth = 1.F;
    // PickPasses[1].stencil = 0;
    PickPasses[1].RenderArea = FVec4(0.F, 0.F, 1280.F, 720.F);  // Разрешение области рендеринга по умолчанию.
    PickPasses[1].ClearColour = FVec4(1.F, 1.F, 1.F, 1.F);
    PickPasses[1].ClearFlags = RenderpassClearFlag::None;
    PickPasses[1].RenderTargetCount = 1;
    PickPasses[1].target.AttachmentCount = 1;
    PickPasses[1].target.attachments = UiPickTargetAttachment.MovePtr();

    AppConfig.RenderViews[3].name = "pick";
    // AppConfig.RenderViews[3].width = 0;
    // AppConfig.RenderViews[3].height = 0;
    AppConfig.RenderViews[3].type = RenderView::KnownTypePick;
    AppConfig.RenderViews[3].VMS = RenderView::ViewMatrixSourceSceneCamera;
    AppConfig.RenderViews[3].PassCount = 2;
    AppConfig.RenderViews[3].passes = PickPasses.MovePtr();

    return true;
}
