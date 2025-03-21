#include "game.h"

#include <core/logger.hpp>
#include <core/input.hpp>
#include <core/metrics.hpp>

#include <systems/camera_system.hpp>
#include <renderer/rendering_system.h>
#include <new>

// ЗАДАЧА: временный код
#include <core/identifier.hpp>
#include <systems/geometry_system.h>
#include <systems/render_view_system.h>
#include <systems/resource_system.h>
#include <renderer/views/render_view_pick.h>
#include "debug_console.h"
// ЗАДАЧА: конец временного кода

bool GameConfigureRenderViews(Application& app);
void ApplicationRegisterEvents(Application& app);
void ApplicationUnregisterEvents(Application& app);
static bool LoadMainScene(Application* app);

bool GameOnEvent(u16 code, void *sender, void *ListenerInst, EventContext context)
{
    auto GameInst = reinterpret_cast<Application*>(ListenerInst);
    auto state = reinterpret_cast<Game*>(GameInst->state);
    switch (code) {
        case EventSystem::OojectHoverIdChanged: {
            state->HoveredObjectID = context.data.u32[0];
            return true;
        }
    }

    return false;
}

// ЗАДАЧА: временно
bool GameOnDebugEvent(u16 code, void *sender, void *ListenerInst, EventContext context)
{
    auto GameInst = reinterpret_cast<Application*>(ListenerInst);
    auto state = reinterpret_cast<Game*>(GameInst->state);
    if (code == EventSystem::DEBUG0) {
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

        // Просто замените материал на первой сетке, если она существует.
        auto& g = state->meshes[0].geometries[0];
        if (g) {
            // Приобретите новый материал.
            g->material = MaterialSystem::Acquire(names[choice]);

        if (!g->material) {
                MWARN("Application::OnDebugEvent: материал не найден! Использование материала по умолчанию.");
                g->material = MaterialSystem::GetDefaultMaterial();
            }

            // Освободите старый рассеянный материал.
            MaterialSystem::Release(OldName);
        }
        return true;
    } else if (code == EventSystem::DEBUG1) {
        if (state->MainScene.state < SimpleScene::State::Loading) {
            MDEBUG("Загрузка основной сцены...");
            
            if (!LoadMainScene(GameInst)) {
                MERROR("Не удалось загрузить основную сцену!");
            }
        }
        return true;
    } else if (code == EventSystem::DEBUG2) {
        if (state->MainScene.state == SimpleScene::State::Loaded) {
            MDEBUG("Выгрузка сцены...");
            state->MainScene.Unload(false);
            MDEBUG("Выполнено.")
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

bool ApplicationBoot(Application& app)
{
    MINFO("Загрузка испытательной сборки...");

    app.state = MemorySystem::Allocate(sizeof(Game), Memory::Game);
    app.state = new(app.state) Game();

    app.AppConfig.AppFrameDataSize = sizeof(TestbedApplicationFrameData);

    app.AppConfig.FontConfig.AutoRelease = false;
    app.AppConfig.FontConfig.DefaultBitmapFontCount = 1;

    // BitmapFontConfig BmpFontConfig { "Metrika 21px", 21, "Metrika" };
    app.AppConfig.FontConfig.BitmapFontConfigs = new BitmapFontConfig("Metrika 21px", 21, "Metrika");

    // SystemFontConfig SysFontConfig { "Noto Sans", 20, "NotoSansCJK" };
    app.AppConfig.FontConfig.DefaultSystemFontCount = 1;
    app.AppConfig.FontConfig.SystemFontConfigs = new SystemFontConfig("Noto Sans", 20, "NotoSansCJK");

    app.AppConfig.FontConfig.MaxBitmapFontCount = app.AppConfig.FontConfig.MaxSystemFontCount = 101;

    // Настроить представления рендеринга. ЗАДАЧА: прочитать из файла?
    if (!GameConfigureRenderViews(app)) {
        MERROR("Не удалось настроить представления рендерера. Прерывание приложения.");
        return false;
    }

    // Раскладки клавиатуры
    GameSetupKeymaps(&app);
    // Консольные команды
    GameSetupCommands();

    return true;
}

bool ApplicationInitialize(Application& app)
{
    MDEBUG("Game::Initialize вызван!");

    ApplicationRegisterEvents(app);
    auto state = reinterpret_cast<Game*>(app.state);
    state->console.Load();

    state->ForwardMoveSpeed  = 5.F;
    state->BackwardMoveSpeed = 2.5F;

    // Сетки мира
    // Отменить все сетки.
    for (u32 i = 0; i < 10; ++i) {
        state->meshes[i].generation = INVALID::U8ID;
        state->UiMeshes[i].generation = INVALID::U8ID;
    }

    if (!state->TestText.Create("testbed_metrika_test_text", TextType::Bitmap, "Metrika 21px", 21, "Какой-то тестовый текст 123,\n\tyo!")) {
        MERROR("Не удалось загрузить базовый растровый текст пользовательского интерфейса.");
        return false;
    }
    // Переместить отладочный текст в новую нижнюю часть экрана.
    state->TestText.SetPosition(FVec3(20.F, state->height - 75.F, 0.F)); 

    if(!state->TestSysText.Create("testbed_UTF_test_text", TextType::System, "Noto Sans CJK JP", 31, "Какой-то тестовый текст 123, \n\tyo!\n\n\tこんにちは 한")) {
        MERROR("Не удалось загрузить базовый системный текст пользовательского интерфейса.");
        return false;
    }
    state->TestSysText.SetPosition(FVec3(500.F, 550.F, 0.F));

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
    state->WorldCamera->SetPosition(FVec3(54.45F, 8.34F, 67.15F));
    state->WorldCamera->SetRotationEuler(FVec3(-11.083F, 262.600F, 0.F));

    state->UpdateClock.Zero();
    state->RenderClock.Zero();
    
    return state->running = true;
}

bool ApplicationUpdate(Application& app, const FrameData& rFrameData)
{
    // auto AppFrameData = reinterpret_cast<TestbedApplicationFrameData*>(rFrameData.ApplicationFrameData);
    auto state = reinterpret_cast<Game*>(app.state);

    if (!state->running) {
        return true;
    }

    state->UpdateClock.Start();

    if (state->MainScene.state >= SimpleScene::State::Loaded) {
        if (!state->MainScene.Update(rFrameData)) {
            MWARN("Не удалось обновить основную сцену.");
        }

        // Выполните небольшой поворот на первой сетке.
        Quaternion rotation( FVec3(0.F, 1.F, 0.F), 0.5F * rFrameData.DeltaTime, false );
        // state->meshes[0].transform.Rotate(rotation);

        // Выполняем аналогичное вращение для второй сетки, если она существует.
        // «Родительский» второй куб по отношению к первому.
        // state->meshes[1].transform.Rotate(rotation);

        // Выполняем аналогичное вращение для третьей сетки, если она существует.
        // «Родительский» второй куб по отношению к первому.
        // state->meshes[2].transform.Rotate(rotation);
        
        if (state->PointLight1) {
            state->PointLight1->data.colour.Set(
                MCLAMP((Math::sin(rFrameData.TotalTime + 0.F)  + 1.F) * 0.5F, 0.F, 1.F),
                MCLAMP((Math::sin(rFrameData.TotalTime + 0.3F) + 1.F) * 0.5F, 0.F, 1.F),
                MCLAMP((Math::sin(rFrameData.TotalTime + 0.6F) + 1.F) * 0.5F, 0.F, 1.F),
                1.F); 
            state->PointLight1->data.position.x = 70.F + Math::sin(rFrameData.TotalTime);
        }
    }

    // Отслеживайте различия в распределении.
    state->PrevAllocCount = state->AllocCount;
    state->AllocCount = MemorySystem::GetMemoryAllocCount();

    // Обновляем растровый текст с позицией камеры. ПРИМЕЧАНИЕ: пока используем камеру по умолчанию.
    auto WorldCamera = CameraSystem::GetDefault();
    const auto& pos = WorldCamera->GetPosition();
    const auto& rot = WorldCamera->GetRotationEuler();

    // Также прикрепите текущее состояние мыши.
    bool LeftDown = InputSystem::IsButtonDown(Buttons::Left);
    bool RightDown = InputSystem::IsButtonDown(Buttons::Right);
    i16 MouseX, MouseY;
    InputSystem::GetMousePosition(MouseX, MouseY);

    // Преобразовать в NDC
    f32 mouseXNdc = Math::RangeConvertF32((f32)MouseX, 0.F, (f32)state->width,  -1.F, 1.F);
    f32 MouseYNdc = Math::RangeConvertF32((f32)MouseY, 0.F, (f32)state->height, -1.F, 1.F);

    f64 fps, FrameTime;
    Metrics::Frame(fps, FrameTime);

    const char* VsyncText = RenderingSystem::FlagEnabled(RenderingConfigFlagBits::VsyncEnabledBit) ? "Вкл" : "Выкл";
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
        rFrameData.DrawnMeshCount,
        state->HoveredObjectID == INVALID::ID ? "none" : "",
        state->HoveredObjectID == INVALID::ID ? 0 : state->HoveredObjectID
    );

    if (state->running) {
        state->TestText.SetText(TextBuffer);
    }
    
    state->console.Update();

    state->UpdateClock.Update();
    state->LastUpdateElapsed = state->UpdateClock.elapsed;

    return true;
}

bool ApplicationRender(Application& app, RenderPacket& packet, FrameData& rFrameData) 
{
    auto state = reinterpret_cast<Game*>(app.state);

    if (!state->running) {
        return true;
    }
    // auto AppFrameData = reinterpret_cast<TestbedApplicationFrameData*>(rFrameData.ApplicationFrameData);

    state->RenderClock.Start();
    // ЗАДАЧА: временный код

    // ЗАДАЧА: Чтение из конфигурации кадра
    packet.ViewCount = 4;
    packet.views = reinterpret_cast<RenderView::Packet*>(rFrameData.FrameAllocator->Allocate(sizeof(RenderView::Packet) * packet.ViewCount));

    // FIXME: Прочитать это из конфигурации
    packet.views[0].view = RenderViewSystem::Get("skybox");
    packet.views[1].view = RenderViewSystem::Get("world");
    packet.views[2].view = RenderViewSystem::Get("ui");
    packet.views[3].view = RenderViewSystem::Get("pick");

    // Даем нашей сцене команду сгенерировать соответствующие пакетные данные.
    
    // Мир 
    // ЗАДАЧА: выполняет поиск в каждом кадре.
    if (state->MainScene.state == SimpleScene::State::Loaded) {
        if (!state->MainScene.PopulateRenderPacket(state->WorldCamera, (f32)state->width / state->height, rFrameData, packet)) {
            MERROR("Не удалось выполнить общедоступный рендеринг пакета для основной сцены.");
            return false;
        }
    }

    // Пользовательский интерфейс
    UiPacketData UiPacket{};
    u32 UIMeshCount = 0;
    u32 MaxUiMeshes = 10;
    auto UIMeshes = reinterpret_cast<Mesh**>(rFrameData.FrameAllocator->Allocate(sizeof(Mesh*) * MaxUiMeshes));
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
    auto& DebugConsoleText = state->console.GetText();
    bool RenderDebugConsole = DebugConsoleText && state->console.Visible();
    if (RenderDebugConsole) {
        UiPacket.TextCount += 2;
    }
    auto texts = reinterpret_cast<Text**>(rFrameData.FrameAllocator->Allocate(sizeof(Text*) * UiPacket.TextCount));
    texts[0] = &state->TestText;
    texts[1] = &state->TestSysText;
    if (RenderDebugConsole) {
        texts[2] = &DebugConsoleText;
        texts[3] = &state->console.GetEntryText();
    }
    UiPacket.texts = texts;
    if (!RenderViewSystem::BuildPacket(RenderViewSystem::Get("ui"), *rFrameData.FrameAllocator, &UiPacket, packet.views[2])) {
        MERROR("Не удалось построить пакет для представления «ui».");
        return false;
    }

    RenderViewPick::PacketData PickPacket;
    PickPacket.UiMeshData = UiPacket.MeshData;
    PickPacket.WorldMeshData = &packet.views[1].geometries;  // ЗАДАЧА: не жестко закодированный индекс?
    PickPacket.texts = UiPacket.texts;
    PickPacket.TextCount = UiPacket.TextCount;
    PickPacket.UiGeometryCount = 0;

    if (!RenderViewSystem::BuildPacket(RenderViewSystem::Get("pick"), *rFrameData.FrameAllocator, &PickPacket, packet.views[3])) {
        MERROR("Не удалось построить пакет для представления «pick».");
        return false;
    }

    // ЗАДАЧА: конец временного кода

    state->RenderClock.Update();
    
    return true;
}

void ApplicationOnResize(Application& app, u32 width, u32 height) 
{
    if (!app.state) {
        return;
    }
    
    auto state = reinterpret_cast<Game*>(app.state);

    state->width = width;
    state->height = height;

    // ЗАДАЧА: временный блок кода
    // Переместить отладочный текст в новую нижнюю часть экрана.
    state->TestText.SetPosition(FVec3(20, state->height - 75, 0));
    // ЗАДАЧА: конец временного блока кода
}

void ApplicationShutdown(Application& app)
{
    auto state = reinterpret_cast<Game*>(app.state);
    state->running = false;

    if (state->MainScene.state == SimpleScene::State::Loaded) {
        MDEBUG("Выгрузка сцены...");

        state->MainScene.Unload(true);

        MDEBUG("Выполнено.");
    }

    // ЗАДАЧА: Временный блок кода

    // Уничтожение текста пользовательского интерфейса
    // state->TestText.~Text();
    // state->TestSysText.~Text();
}

void ApplicationLibOnLoad(Application& app)
{
    ApplicationRegisterEvents(app);
    reinterpret_cast<Game*>(app.state)->console.OnLibLoad(app.stage >= Application::Stage::BootComplete);
    if (app.stage >= Application::Stage::BootComplete) {
        GameSetupCommands();
        GameSetupKeymaps(&app);
    }
}

void ApplicationLibOnUnload(Application& app)
{
    ApplicationUnregisterEvents(app);
    reinterpret_cast<Game*>(app.state)->console.OnLibUnload();
    GameRemoveCommands();
    GameRemoveKeymaps(&app);
}

bool GameConfigureRenderViews(Application& app)
{
    app.AppConfig.RenderViews.Resize(4);

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
    SkyboxPasses[0].RenderTargetCount = RenderingSystem::WindowAttachmentCountGet();
    SkyboxPasses[0].target.AttachmentCount = 1;
    SkyboxPasses[0].target.attachments = SkyboxTargetAttachment.MovePtr();

    // Skybox view
    app.AppConfig.RenderViews[0].name = "skybox";
    // app.AppConfig.RenderViews[0].CustomShaderName
    // app.AppConfig.RenderViews[0].width = 0;
    // app.AppConfig.RenderViews[0].height = 0;
    app.AppConfig.RenderViews[0].type = RenderView::KnownTypeSkybox;
    app.AppConfig.RenderViews[0].VMS = RenderView::ViewMatrixSourceSceneCamera;
    app.AppConfig.RenderViews[0].PassCount = 1;
    app.AppConfig.RenderViews[0].passes = SkyboxPasses.MovePtr();

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
    WorldPasses[0].RenderTargetCount = RenderingSystem::WindowAttachmentCountGet();
    WorldPasses[0].target.AttachmentCount = 2;
    WorldPasses[0].target.attachments = WorldTargetAttachment.MovePtr();

    app.AppConfig.RenderViews[1].name = "world";
    // app.AppConfig.RenderViews[1].width = 0;
    // app.AppConfig.RenderViews[1].height = 0;
    app.AppConfig.RenderViews[1].type = RenderView::KnownTypeWorld;
    app.AppConfig.RenderViews[1].VMS = RenderView::ViewMatrixSourceSceneCamera;
    app.AppConfig.RenderViews[1].PassCount = 1;
    app.AppConfig.RenderViews[1].passes = WorldPasses.MovePtr();

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
    UIPasses[0].RenderTargetCount = RenderingSystem::WindowAttachmentCountGet();
    UIPasses[0].target.AttachmentCount = 1;
    UIPasses[0].target.attachments = UiTargetAttachment.MovePtr();

    app.AppConfig.RenderViews[2].name = "ui";
    // app.AppConfig.RenderViews[2].width = 0;
    // app.AppConfig.RenderViews[2].height = 0;
    app.AppConfig.RenderViews[2].type = RenderView::KnownTypeUI;
    app.AppConfig.RenderViews[2].VMS = RenderView::ViewMatrixSourceUiCamera;
    app.AppConfig.RenderViews[2].PassCount = 1;
    app.AppConfig.RenderViews[2].passes = UIPasses.MovePtr();

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

    app.AppConfig.RenderViews[3].name = "pick";
    // app.AppConfig.RenderViews[3].width = 0;
    // app.AppConfig.RenderViews[3].height = 0;
    app.AppConfig.RenderViews[3].type = RenderView::KnownTypePick;
    app.AppConfig.RenderViews[3].VMS = RenderView::ViewMatrixSourceSceneCamera;
    app.AppConfig.RenderViews[3].PassCount = 2;
    app.AppConfig.RenderViews[3].passes = PickPasses.MovePtr();

    return true;
}

static void ToggleVsync() {
    bool VsyncEnabled = RenderingSystem::FlagEnabled(RenderingConfigFlagBits::VsyncEnabledBit);
    VsyncEnabled = !VsyncEnabled;
    RenderingSystem::FlagSetEnabled(RenderingConfigFlagBits::VsyncEnabledBit, VsyncEnabled);
}

static bool GameOnMVarChanged(u16 code, void* sender, void* ListenerInst, EventContext data) {
    if (code == EventSystem::MVarChanged && MString::Equali(data.data.c, "vsync")) {
        ToggleVsync();
    }
    return false;
}

void ApplicationRegisterEvents(Application &app)
{
    //ЗАДАЧА: временно
    EventSystem::Register(EventSystem::DEBUG0, &app, GameOnDebugEvent);
    EventSystem::Register(EventSystem::DEBUG1, &app, GameOnDebugEvent);
    EventSystem::Register(EventSystem::DEBUG2, &app, GameOnDebugEvent);
    EventSystem::Register(EventSystem::OojectHoverIdChanged, &app, GameOnEvent);
    //ЗАДАЧА: временно

    EventSystem::Register(EventSystem::KeyPressed,  &app, GameOnKey);
    EventSystem::Register(EventSystem::KeyReleased, &app, GameOnKey);
    EventSystem::Register(EventSystem::MVarChanged, nullptr, GameOnMVarChanged);
}

void ApplicationUnregisterEvents(Application &app)
{
    EventSystem::Unregister(EventSystem::DEBUG0, &app, GameOnDebugEvent);
    EventSystem::Unregister(EventSystem::DEBUG1, &app, GameOnDebugEvent);
    EventSystem::Unregister(EventSystem::DEBUG2, &app, GameOnDebugEvent);
    EventSystem::Unregister(EventSystem::OojectHoverIdChanged, &app, GameOnEvent);
    // ЗАДАЧА: конец временного блока кода

    EventSystem::Unregister(EventSystem::KeyPressed, &app, GameOnKey);
    EventSystem::Unregister(EventSystem::KeyReleased, &app, GameOnKey);
    EventSystem::Unregister(EventSystem::MVarChanged, nullptr, GameOnMVarChanged);
}

bool LoadMainScene(Application *app)
{
    auto state = reinterpret_cast<Game*>(app->state);

    // Загрузить файл конфигурации
    // ЗАДАЧА: очистить ресурс.
    SimpleSceneResource simpleSceneResource;
    if (!ResourceSystem::Load("test_scene", eResource::SimpleScene, nullptr, simpleSceneResource)) {
        MERROR("Не удалось загрузить файл сцены, проверьте журналы выше.");
        return false;
    }

    // ЗАДАЧА: временная загрузка/подготовка материала
    if (!state->MainScene.Create(&simpleSceneResource.data)) {
        MERROR("Не удалось создать основную сцену");
        return false;
    }

    // Добавить объекты в сцену
    // Загрузить конфигурацию куба и загрузить геометрию из него.
    // Mesh::Config Cube0_Config = {0};
    // Cube0_Config.GeometryCount = 1;
    // Cube0_Config.GConfigs = new GeometryConfig();
    // *Cube0_Config.GConfigs = GeometrySystem::GenerateCubeConfig(10.F, 10.F, 10.F, 1.F, 1.F, "test_cube", "test_material");

    // if (!state->meshes[0].Create(Cube0_Config)) {
    //     MERROR("Не удалось создать сетку для куба 0");
    //     return false;
    // }
    // state->meshes[0].transform = Transform();
    // state->MainScene.AddMesh(state->meshes[0]);

    // Второй куб
    // Mesh::Config Cube1_Config = {0};
    // Cube1_Config.GeometryCount = 1;
    // Cube1_Config.GConfigs = new GeometryConfig();
    // *Cube1_Config.GConfigs = GeometrySystem::GenerateCubeConfig(5.F, 5.F, 5.F, 1.F, 1.F, "test_cube_2", "test_material");

    // if (!state->meshes[1].Create(Cube1_Config)) {
    //     MERROR("Не удалось создать сетку для куба 1");
    //     return false;
    // }
    // state->meshes[1].transform = Transform(FVec3(10.F, 0.F, 1.F));
    // state->meshes[1].transform.SetParent(&state->meshes[0].transform);

    // state->MainScene.AddMesh(state->meshes[1]);

    // Третий куб!
    // Mesh::Config Сube2_Сonfig = {0};
    // Сube2_Сonfig.GeometryCount = 1;
    // Сube2_Сonfig.GConfigs = new GeometryConfig();
    // Сube2_Сonfig.GConfigs[0] = GeometrySystem::GenerateCubeConfig(2.F, 2.F, 2.F, 1.F, 1.F, "test_cube_2", "test_material");

    // if (!state->meshes[2].Create(Сube2_Сonfig)) {
    //     MERROR("Не удалось создать сетку для куба 2");
    //     return false;
    // }
    // state->meshes[2].transform = Transform(FVec3(5.F, 0.F, 1.F));
    // state->meshes[2].transform.SetParent(&state->meshes[1].transform);

    // state->MainScene.AddMesh(state->meshes[2]);

    // Инициализация
    if (!state->MainScene.Initialize()) {
        MERROR("Не удалось инициализировать основную сцену, прерывание игры.");
        return false;
    }

    state->PointLight1 = state->MainScene.GetPointLight("point_light_1");

    // Фактически загрузить сцену.
    return state->MainScene.Load();
}
