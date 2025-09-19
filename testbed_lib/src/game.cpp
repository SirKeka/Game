#include "game.h"

#include <core/logger.hpp>
#include <core/input.h>
#include <core/metrics.hpp>

#include <systems/camera_system.hpp>
#include <renderer/renderpass.h>
#include <new>

#include <math/geometry_utils.h>

// ЗАДАЧА: Реедактор временно
#include "editor/gizmo.h"
#include "editor/render_view_editor_world.h"

// ЗАДАЧА: временный код
#include <core/identifier.h>
#include <systems/geometry_system.h>
#include <systems/render_view_system.h>
#include <systems/resource_system.h>
#include "views/render_view_pick.h"
#include "views/render_view_skybox.h"
#include "views/render_view_ui.h"
#include "views/render_view_world.h"
#include "debug_console.h"
// ЗАДАЧА: конец временного кода

bool GameConfigureRenderViews(ApplicationConfig& config);
void ApplicationRegisterEvents(Application& app);
void ApplicationUnregisterEvents(Application& app);
static bool LoadMainScene(Application* app);

void Game::ClearDebugObjects() 
{
    if (TestBoxes) {
        const u32& BoxCount = TestBoxes.Length();
        for (u32 i = 0; i < BoxCount; ++i) {
            auto& box = TestBoxes[i];
            box.Unload();
            box.Destroy();
        }
        TestBoxes.Clear();
    }

    if (TestLines) {
        const u32& LineCount = TestLines.Length();
        for (u32 i = 0; i < LineCount; ++i) {
            auto line = TestLines[i];
            line.Unload();
            line.Destroy();
        }
        TestLines.Clear();
    }
}

static bool GameOnEvent(u16 code, void *sender, void *ListenerInst, EventContext context)
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
static bool GameOnDebugEvent(u16 code, void *sender, void *ListenerInst, EventContext context)
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
            state->ClearDebugObjects();
            MDEBUG("Выполнено.")
        }
        return true;
    }

    return true;
}
//ЗАДАЧА: временно

static bool GameOnKey(u16 code, void *sender, void *ListenerInst, EventContext context)
{
    auto GameInst = (Application*)ListenerInst;
    auto state = (Game*)GameInst->state;
    if (code == EventSystem::KeyReleased) {
        Keys KeyCode = (Keys)context.data.u16[0];
        // Измените ориентацию гизмо.
        if (KeyCode == Keys::G) {
            u16 orientation = state->gizmo.GetOrientation();
            orientation++;
            if (orientation > Gizmo::Orientation::Max) {
                orientation = Gizmo::Orientation::Global;
            }
            state->gizmo.SetOrientation(static_cast<Gizmo::Orientation>(orientation));
        }
    }

    return false;
}

static bool GameOnDrag(u16 code, void* sender, void* ListenerInst, EventContext context)
{
    i16 x = context.data.i16[0];
    i16 y = context.data.i16[1];
    Buttons DragButton = static_cast<Buttons>(context.data.u16[2]);
    auto state = (Game*)ListenerInst;

    // Обращайте внимание только на перетаскивание левой кнопкой.
    if (DragButton == Buttons::Left) {
        auto view = state->WorldCamera->GetView();
        auto origin = state->WorldCamera->GetPosition();

        // ЗАДАЧА: Получите это из области просмотра.
        auto ProjectionMatrix = Matrix4D::MakeFrustumProjection(Math::DegToRad(45.F), (f32)state->width / state->height, 0.1F, 4000.F);

        Ray ray {
            FVec2((f32)x, (f32)y),
            FVec2((f32)state->width, (f32)state->height),
            origin,
            view,
            ProjectionMatrix
        };

        if (code == EventSystem::MouseDragBegin) {
            state->UsingGizmo = true;
            // Начало перетаскивания — измените режим взаимодействия на «перетаскивание».
            state->gizmo.InteractionBegin(state->WorldCamera, ray, Gizmo::InteractionType::MouseDrag);
        } else if (code == EventSystem::MouseDragged) {
            state->gizmo.HandleInteraction(state->WorldCamera, ray, Gizmo::InteractionType::MouseDrag);
        } else if (code == EventSystem::MouseDragEnd) {
            state->gizmo.InteractionEnd();
            state->UsingGizmo = false;
        }
    }

    return false;  // Позвольте другим обработчикам обрабатывать.
}

static bool GameOnButton(u16 code, void* sender, void* ListenerInst, EventContext context) {
    if (code == EventSystem::ButtonPressed) {
        //
    } else if (code == EventSystem::ButtonReleased) {
        Buttons button = (Buttons)context.data.u16[0];
        switch (button) {
            case Buttons::Left: {
                i16 x = context.data.i16[1];
                i16 y = context.data.i16[2];
                auto state = (Game*)ListenerInst;

                // Если сцена не загружена, не делайте ничего другого.
                if (state->MainScene.state < SimpleScene::State::Loaded) {
                    return false;
                }

                // Если вы «манипулируете штуковиной», не следуйте приведенной ниже логике.
                if (state->UsingGizmo) {
                    return false;
                }

                auto view = state->WorldCamera->GetView();
                auto origin = state->WorldCamera->GetPosition();

                // ЗАДАЧА: Получите это из области просмотра.
                auto ProjectionMatrix = Matrix4D::MakeFrustumProjection(Math::DegToRad(45.F), (f32)state->width / state->height, 0.1F, 4000.F);
                Ray ray {
                    FVec2((f32)x, (f32)y),
                    FVec2((f32)state->width, (f32)state->height),
                    origin,
                    view,
                    ProjectionMatrix
                };

                RaycastResult RayResult;
                if (state->MainScene.Raycast(ray, RayResult)) {
                    const u32& HitCount = RayResult.hits.Length();
                    for (u32 i = 0; i < HitCount; ++i) {
                        auto& hit = RayResult.hits[i];
                        MINFO("Попадание! id: %u, дистанция: %f", hit.UniqueID, hit.distance);

                        // Создайте линию отладки в точке начала и конца луча (на пересечении).
                        DebugLine3D TestLine {ray.origin, hit.position};
                        TestLine.Initialize();
                        TestLine.Load();
                        // Жёлтый — для попаданий.
                        TestLine.SetColour(FVec4(1.F, 1.F, 0.F, 1.F));

                        state->TestLines.PushBack(TestLine);

                        // Создайте поле отладки для отображения точки пересечения.
                        DebugBox3D TestBox {FVec3(0.1F, 0.1F, 0.1F)};
                        TestBox.Initialize();
                        TestBox.Load();

                        Extents3D ext;
                        ext.min = FVec3(hit.position.x - 0.05F, hit.position.y - 0.05F, hit.position.z - 0.05F);
                        ext.max = FVec3(hit.position.x + 0.05F, hit.position.y + 0.05F, hit.position.z + 0.05F);
                        TestBox.SetExtents(ext);

                        state->TestBoxes.PushBack(TestBox);

                        // Выбор объекта
                        if (i == 0) {
                            state->selection.UniqueID = hit.UniqueID;
                            state->selection.xform = state->MainScene.GetTransform(hit.UniqueID);
                            if (state->selection.xform) {
                                MINFO("Selected object id %u", hit.UniqueID);
                                // state->gizmo.SelectedXform = state->selection.xform;
                                state->gizmo.SetSelectedTransform(state->selection.xform);
                                // state->gizmo.xform.SetParent(state->selection.xform);
                            }
                        }
                    }
                } else {
                    MINFO("Нет попадания");

                    // Создайте отладочную строку, где начинается и продолжается испускание луча.
                    DebugLine3D TestLine{ray.origin, ray.origin + ray.direction * 100.F};
                    TestLine.Initialize();
                    TestLine.Load();
                    // Пурпурный — для случаев отсутствия попадания.
                    TestLine.SetColour(FVec4(1.F, 0.F, 1.F, 1.F));

                    state->TestLines.PushBack(TestLine);

                    if (state->selection.xform) {
                        MINFO("Object deselected.");
                        state->selection.xform = nullptr;
                        state->selection.UniqueID = INVALID::ID;

                        state->gizmo.SetSelectedTransform();
                    }

                    // ЗАДАЧА: скрыть гаджет, отключить ввод и т. д.
                }

            } break;
            default: break;
        }
    }

    return false;
}

static bool GameOnMouseMove(u16 code, void* sender, void* ListenerInst, EventContext context) {
    if (code == EventSystem::MouseMoved && !InputSystem::IsButtonDragging(Buttons::Left)) {
        i16 x = context.data.i16[0];
        i16 y = context.data.i16[1];

        auto state = (Game*)ListenerInst;

        const auto& view = state->WorldCamera->GetView();
        const auto& origin = state->WorldCamera->GetPosition();

        // ЗАДАЧА: Получить это из области просмотра.
        auto ProjectionMatrix = Matrix4D::MakeFrustumProjection(Math::DegToRad(45.F), (f32)state->width / state->height, 0.1F, 4000.F);

        Ray ray {
            FVec2((f32)x, (f32)y),
            FVec2((f32)state->width, (f32)state->height),
            origin,
            view,
            ProjectionMatrix
        };

        state->gizmo.HandleInteraction(state->WorldCamera, ray, Gizmo::InteractionType::MouseHover);
    }
    return false;  // Разрешить другим обработчикам событий получать это событие.
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
    if (!GameConfigureRenderViews(app.AppConfig)) {
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
    ResourceSystem::RegisterLoader(eResource::Type::SimpleScene, MString(), "scenes");

    auto state = reinterpret_cast<Game*>(app.state);

    state->selection.UniqueID = INVALID::ID;
    state->selection.xform = nullptr;
    state->console.Load();

    state->ForwardMoveSpeed  = 5.F;
    state->BackwardMoveSpeed = 2.5F;

    // Настройка редактора gizmo.
    /* if (!state->gizmo.Create()) {
        MERROR("Не удалось создать редактор!");
        return false;
    } */

    if (!state->gizmo.Initialize()) {
        MERROR("Не удалось инициализировать редактор!");
        return false;
    }

    if (!state->gizmo.Load()) {
        MERROR("Не удалось загрузить редактор!");
        return false;
    }

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
    uiverts[3].position.y = 0.F;
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
    state->UiMeshes[0].geometries = new Geometry*[1];
    state->UiMeshes[0].geometries[0] = GeometrySystem::Acquire(UI_Config, true);
    state->UiMeshes[0].transform = Transform();
    state->UiMeshes[0].generation = 0;

    state->UiMeshes[0].transform.Translate(FVec3(650, 5, 0));
    
    state->WorldCamera = CameraSystem::GetDefault();
    state->WorldCamera->SetPosition(FVec3(16.07F, 4.5F, 25.F));
    state->WorldCamera->SetRotationEuler(FVec3(-20.F, 51.F, 0.F));

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

        state->gizmo.Update();
        // Выполните небольшой поворот на первой сетке.
        // Quaternion rotation( FVec3(0.F, 1.F, 0.F), 0.5F * rFrameData.DeltaTime, false );
        // state->meshes[0].transform.Rotate(rotation);

        // Выполняем аналогичное вращение для второй сетки, если она существует.
        // «Родительский» второй куб по отношению к первому.
        // state->meshes[1].transform.Rotate(rotation);

        // Выполняем аналогичное вращение для третьей сетки, если она существует.
        // «Родительский» второй куб по отношению к первому.
        // state->meshes[2].transform.Rotate(rotation);
        
        if (state->PointLight1) {
            state->PointLight1->data.colour.Set(
                MCLAMP(Math::sin(rFrameData.TotalTime) * 0.75F + 0.5F, 0.F, 1.F),
                MCLAMP(Math::sin(rFrameData.TotalTime + M_2PI / 3) * 0.75 + 0.5F, 0.F, 1.F),
                MCLAMP(Math::sin(rFrameData.TotalTime + M_4PI / 3) * 0.75 + 0.5F, 0.F, 1.F),
                1.F); 
            state->PointLight1->data.position.z = 20.F + Math::sin(rFrameData.TotalTime);
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

    packet.ViewCount = 5;
    packet.views = reinterpret_cast<RenderViewPacket*>(rFrameData.FrameAllocator->Allocate(sizeof(RenderViewPacket) * packet.ViewCount));

    // FIXME: Прочитать это из конфигурации
    packet.views[Testbed::PacketViews::Skybox].view      = RenderViewSystem::Get("skybox");
    packet.views[Testbed::PacketViews::World].view       = RenderViewSystem::Get("world");
    packet.views[Testbed::PacketViews::EditorWorld].view = RenderViewSystem::Get("editor_world");
    packet.views[Testbed::PacketViews::UI].view          = RenderViewSystem::Get("ui");
    packet.views[Testbed::PacketViews::Pick].view        = RenderViewSystem::Get("pick");

    // Даем нашей сцене команду сгенерировать соответствующие пакетные данные. ПРИМЕЧАНИЕ: Генерирует пакеты скайбокса и мира.
    
    // Мир 
    // ЗАДАЧА: выполняет поиск в каждом кадре.
    if (state->MainScene.state == SimpleScene::State::Loaded) {
        if (!state->MainScene.PopulateRenderPacket(state->WorldCamera, (f32)state->width / state->height, rFrameData, packet)) {
            MERROR("Не удалось выполнить общедоступный рендеринг пакета для основной сцены.");
            return false;
        }
    }

    // HACK: Внедрение отладочной геометрии в пакет мира.
    if (state->MainScene.state == SimpleScene::State::Loaded) {
        const u32& LineCount = state->TestLines.Length();
        for (u32 i = 0; i < LineCount; ++i) {
            GeometryRenderData rdata;
            rdata.model = state->TestLines[i].xform.GetWorld();
            rdata.geometry = &state->TestLines[i].geometry;
            rdata.UniqueID = INVALID::ID;
            packet.views[Testbed::World].DebugGeometries.PushBack(rdata);
        }

        const u32& BoxCount = state->TestBoxes.Length();
        for (u32 i = 0; i < BoxCount; ++i) {
            GeometryRenderData rData;
            rData.model = state->TestBoxes[i].xform.GetWorld();
            rData.geometry = &state->TestBoxes[i].geometry;
            rData.UniqueID = INVALID::ID;
            packet.views[Testbed::World].DebugGeometries.PushBack(rData);
        }
    }

    // Редактор мира
    {
        auto& ViewPacket = packet.views[Testbed::PacketViews::EditorWorld];
        const auto view = ViewPacket.view;

        EditorWorldPacketData EditorWorldData{};
        EditorWorldData.gizmo = &state->gizmo;
        if (!RenderViewSystem::BuildPacket(view, *rFrameData.FrameAllocator, &EditorWorldData, ViewPacket)) {
            MERROR("Не удалось построить пакет для представления «editor_world».");
            return false;
        }
    }

    // Пользовательский интерфейс
    UiPacketData UiPacket{};
    {
        auto& ViewPacket = packet.views[Testbed::PacketViews::UI];
        const auto view = ViewPacket.view;

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
        if (!RenderViewSystem::BuildPacket(view, *rFrameData.FrameAllocator, &UiPacket, ViewPacket)) {
            MERROR("Не удалось построить пакет для представления «ui».");
            return false;
        }
    }

    {
        auto& ViewPacket = packet.views[Testbed::PacketViews::Pick];
        const auto view = ViewPacket.view;

        RenderViewPick::PacketData PickPacket;
        PickPacket.UiMeshData = UiPacket.MeshData;
        PickPacket.WorldMeshData = &packet.views[1].geometries;  // ЗАДАЧА: не жестко закодированный индекс?
        PickPacket.TerrainMeshData = &packet.views[1].TerrainGeometries;
        PickPacket.texts = UiPacket.texts;
        PickPacket.TextCount = UiPacket.TextCount;
        PickPacket.UiGeometryCount = 0;

        if (!RenderViewSystem::BuildPacket(view, *rFrameData.FrameAllocator, &PickPacket, ViewPacket)) {
            MERROR("Не удалось построить пакет для представления «pick».");
            return false;
        }
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
        state->ClearDebugObjects();

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

bool GameConfigureRenderViews(ApplicationConfig& config)
{
    config.RenderViews.Resize(5);

    // Skybox view
    {
        auto& SkyboxView = config.RenderViews[0];
        SkyboxView.name = "skybox";
        SkyboxView.RenderpassCount = 1; 
        SkyboxView.passes = (Renderpass*)MemorySystem::Allocate(sizeof(Renderpass) * SkyboxView.RenderpassCount, Memory::Array, true);

        // Конфигурация подпрохода рендеринга
        RenderpassConfig SkyboxPass;
        SkyboxPass.name                   = "Renderpass.Bultin.Skybox";
        SkyboxPass.depth                  = 1.F;
        SkyboxPass.stencil                = 0;
        SkyboxPass.RenderArea             = FVec4(0.F, 0.F, (f32)config.StartWidth, (f32)config.StartHeight);
        SkyboxPass.ClearColour            = FVec4(0.F, 0.F, 0.2F, 1.F);
        SkyboxPass.ClearFlags             = RenderpassClearFlag::ColourBuffer;
        SkyboxPass.RenderTargetCount      = RenderingSystem::WindowAttachmentCountGet();
        SkyboxPass.target.AttachmentCount = 1;
        SkyboxPass.target.attachments     = (RenderTargetAttachmentConfig*)MemorySystem::Allocate(sizeof(RenderTargetAttachmentConfig) * SkyboxPass.target.AttachmentCount, Memory::Array);

        // Цветовое вложение(Colour attachment)
        auto& SkyboxTargetColour = SkyboxPass.target.attachments[0];
        SkyboxTargetColour.type           = RenderTargetAttachmentType::Colour;
        SkyboxTargetColour.source         = RenderTargetAttachmentSource::Default;
        SkyboxTargetColour.LoadOperation  = RenderTargetAttachmentLoadOperation::DontCare;
        SkyboxTargetColour.StoreOperation = RenderTargetAttachmentStoreOperation::Store;
        SkyboxTargetColour.PresentAfter   = false;

        if (!RenderingSystem::RenderpassCreate(SkyboxPass, *SkyboxView.passes)) { // 
            MERROR("Skybox view — Не удалось создать проход рендеринга «%s»",  config.RenderViews[0].passes[0].name.c_str());
            return false;
        }

        SkyboxView.OnRegistered = RenderViewSkybox::OnRegistered;
        SkyboxView.Destroy      = RenderViewSkybox::Destroy;
        SkyboxView.Resize       = RenderViewSkybox::Resize;
        SkyboxView.BuildPacket  = RenderViewSkybox::BuildPacket;
        SkyboxView.Render       = RenderViewSkybox::Render;
    }

    // World view
    {    
        auto& WorldView = config.RenderViews[1];
        WorldView.name = "world";
        // WorldView.width = 0;
        // WorldView.height = 0;
        WorldView.RenderpassCount = 1;
        WorldView.passes = (Renderpass*)MemorySystem::Allocate(sizeof(Renderpass) * WorldView.RenderpassCount, Memory::Array, true);
        
        // Конфигурация подпрохода рендеринга
        RenderpassConfig WorldPass;
        WorldPass.name                   = "Renderpass.Bultin.World";
        WorldPass.depth                  = 1.F;
        WorldPass.stencil                = 0;
        WorldPass.RenderArea             = FVec4(0.F, 0.F, (f32)config.StartWidth, (f32)config.StartHeight); // Разрешение области рендеринга по умолчанию.
        WorldPass.ClearColour            = FVec4(0.F, 0.F, 0.2F, 1.F); 
        WorldPass.ClearFlags             = RenderpassClearFlag::DepthBuffer | RenderpassClearFlag::StencilBuffer;
        WorldPass.RenderTargetCount      = RenderingSystem::WindowAttachmentCountGet();
        WorldPass.target.AttachmentCount = 2;
        WorldPass.target.attachments     = (RenderTargetAttachmentConfig*)MemorySystem::Allocate(sizeof(RenderTargetAttachmentConfig) * WorldPass.target.AttachmentCount, Memory::Array);

        // Привязка цвета
        auto& WorldTargetColour = WorldPass.target.attachments[0];
        WorldTargetColour.type           = RenderTargetAttachmentType::Colour;
        WorldTargetColour.source         = RenderTargetAttachmentSource::Default;
        WorldTargetColour.LoadOperation  = RenderTargetAttachmentLoadOperation::Load;
        WorldTargetColour.StoreOperation = RenderTargetAttachmentStoreOperation::Store;
        WorldTargetColour.PresentAfter   = false;

        // Привязка глубины
        auto& WorldTargetDepth = WorldPass.target.attachments[1];
        WorldTargetDepth.type            = RenderTargetAttachmentType::Depth;
        WorldTargetDepth.source          = RenderTargetAttachmentSource::Default;
        WorldTargetDepth.LoadOperation   = RenderTargetAttachmentLoadOperation::DontCare;
        WorldTargetDepth.StoreOperation  = RenderTargetAttachmentStoreOperation::Store;
        WorldTargetDepth.PresentAfter    = false;

        if (!RenderingSystem::RenderpassCreate(WorldPass, *WorldView.passes)) {
            MERROR("World view — Не удалось создать проход рендеринга «%s»", WorldView.passes[0].name.c_str());
            return false;
        };

        WorldView.OnRegistered = RenderViewWorld::OnRegistered;
        WorldView.Destroy      = RenderViewWorld::Destroy;
        WorldView.Resize       = RenderViewWorld::Resize;
        WorldView.BuildPacket  = RenderViewWorld::BuildPacket;
        WorldView.Render       = RenderViewWorld::Render;
    }

    // ЗАДАЧА: Редактор временно
    // Editor World view.
    {
        auto& EditorWorldView = config.RenderViews[2];
        EditorWorldView.name = "editor_world";
        EditorWorldView.RenderpassCount = 1;
        EditorWorldView.passes = (Renderpass*)MemorySystem::Allocate(sizeof(Renderpass) * EditorWorldView.RenderpassCount, Memory::Array, true);

        // Конфигурация подпрохода рендеринга
        RenderpassConfig EditorWorldPass;
        EditorWorldPass.name = "Renderpass.Testbed.EditorWorld";
        EditorWorldPass.depth = 1.F;
        EditorWorldPass.stencil = 0;
        EditorWorldPass.RenderArea = FVec4(0, 0, (f32)config.StartWidth, (f32)config.StartHeight);  // Разрешение области рендеринга по умолчанию.
        EditorWorldPass.ClearColour = FVec4(0.F, 0.F, 0.F, 1.F);
        EditorWorldPass.ClearFlags = RenderpassClearFlag::DepthBuffer | RenderpassClearFlag::StencilBuffer;
        EditorWorldPass.RenderTargetCount = RenderingSystem::WindowAttachmentCountGet();
        EditorWorldPass.target.AttachmentCount = 2;
        EditorWorldPass.target.attachments = (RenderTargetAttachmentConfig*)MemorySystem::Allocate(sizeof(RenderTargetAttachmentConfig) * EditorWorldPass.target.AttachmentCount, Memory::Array);

        // Привязка цвета
        auto& EditorWorldTargetColour = EditorWorldPass.target.attachments[0];
        EditorWorldTargetColour.type = RenderTargetAttachmentType::Colour;
        EditorWorldTargetColour.source = RenderTargetAttachmentSource::Default;
        EditorWorldTargetColour.LoadOperation = RenderTargetAttachmentLoadOperation::Load;
        EditorWorldTargetColour.StoreOperation = RenderTargetAttachmentStoreOperation::Store;
        EditorWorldTargetColour.PresentAfter = false;

        // Привязка глубины
        auto& EditorWorldTargetDepth = EditorWorldPass.target.attachments[1];
        EditorWorldTargetDepth.type = RenderTargetAttachmentType::Depth;
        EditorWorldTargetDepth.source = RenderTargetAttachmentSource::Default;;
        EditorWorldTargetDepth.LoadOperation = RenderTargetAttachmentLoadOperation::DontCare;
        EditorWorldTargetDepth.StoreOperation = RenderTargetAttachmentStoreOperation::Store;
        EditorWorldTargetDepth.PresentAfter = false;

        if (!RenderingSystem::RenderpassCreate(EditorWorldPass, EditorWorldView.passes[0])) {
            MERROR("EditorWorldView — Не удалось создать проход рендеринга «%s»", EditorWorldView.passes[0].name.c_str());
            return false;
        }

        // Назначьте указатели на функции.
        EditorWorldView.OnRegistered = RenderViewEditorWorld::OnRegistered;
        EditorWorldView.Destroy = RenderViewEditorWorld::Destroy;
        EditorWorldView.Resize = RenderViewEditorWorld::Resize;
        EditorWorldView.BuildPacket = RenderViewEditorWorld::BuildPacket;
        EditorWorldView.Render = RenderViewEditorWorld::Render;
    }

    // UI view
    {
        auto& UiView = config.RenderViews[3];
        UiView.name = "ui";
        // UiView.width = 0;
        // UiView.height = 0;
        UiView.RenderpassCount = 1;
        UiView.passes = (Renderpass*)MemorySystem::Allocate(sizeof(Renderpass) * UiView.RenderpassCount, Memory::Array, true);

        // Конфигурация подпрохода рендеринга
        RenderpassConfig UiPass {};
        UiPass.name                   = "Renderpass.Bultin.UI";
        UiPass.depth                  = 1.F;
        // UiPass.stencil = 0;
        UiPass.RenderArea             = FVec4(0.F, 0.F, 1280.F, 720.F); // Разрешение области рендеринга по умолчанию.
        UiPass.ClearColour            = FVec4(0.F, 0.F, 0.2F, 1.F);
        UiPass.ClearFlags             = RenderpassClearFlag::None;
        UiPass.RenderTargetCount      = RenderingSystem::WindowAttachmentCountGet();
        UiPass.target.AttachmentCount = 1;
        UiPass.target.attachments     = (RenderTargetAttachmentConfig*)MemorySystem::Allocate(sizeof(RenderTargetAttachmentConfig) * UiPass.target.AttachmentCount, Memory::Array);

        // Привязка цвета
        auto& UiTargetAttachment = UiPass.target.attachments[0];
        UiTargetAttachment.type           = RenderTargetAttachmentType::Colour;
        UiTargetAttachment.source         = RenderTargetAttachmentSource::Default;
        UiTargetAttachment.LoadOperation  = RenderTargetAttachmentLoadOperation::Load;
        UiTargetAttachment.StoreOperation = RenderTargetAttachmentStoreOperation::Store;
        UiTargetAttachment.PresentAfter   = true;

        if (!RenderingSystem::RenderpassCreate(UiPass, *UiView.passes)) {
            MERROR("UI view — Не удалось создать проход рендеринга «%s»", UiView.passes[0].name.c_str());
            return false;
        };

        UiView.OnRegistered = RenderViewUI::OnRegistered;
        UiView.Destroy      = RenderViewUI::Destroy;
        UiView.Resize       = RenderViewUI::Resize;
        UiView.BuildPacket  = RenderViewUI::BuildPacket;
        UiView.Render       = RenderViewUI::Render;
    }

    // Pick view
    {
        auto& PickView = config.RenderViews[4];
        PickView.name = "pick";
        // PickView.width = 0;
        // PickView.height = 0;
        PickView.RenderpassCount = 2;
        PickView.passes = (Renderpass*)MemorySystem::Allocate(sizeof(Renderpass) * PickView.RenderpassCount, Memory::Array, true);

        RenderpassConfig WorldPickPass;
        WorldPickPass.name                   = "Renderpass.Builtin.WorldPick";
        WorldPickPass.depth                  = 1.F;
        WorldPickPass.stencil                = 0;
        WorldPickPass.RenderArea             = FVec4(0.F, 0.F, (f32)config.StartWidth, (f32)config.StartHeight);  // Разрешение области рендеринга по умолчанию.
        WorldPickPass.ClearColour            = FVec4(1.F, 1.F, 1.F, 1.F);       // ХАК: очищаем до белого цвета для лучшей видимости// ЗАДАЧА: очищаем до черного цвета, так как 0 — недопустимый идентификатор.
        WorldPickPass.ClearFlags             = RenderpassClearFlag::ColourBuffer | RenderpassClearFlag::DepthBuffer;
        WorldPickPass.RenderTargetCount      = 1;
        WorldPickPass.target.AttachmentCount = 2;
        WorldPickPass.target.attachments     = (RenderTargetAttachmentConfig*)MemorySystem::Allocate(sizeof(RenderTargetAttachmentConfig) * WorldPickPass.target.AttachmentCount, Memory::Array);

        // Привязка цвета
        auto& WorldPickPassColour = WorldPickPass.target.attachments[0];
        WorldPickPassColour.type           = RenderTargetAttachmentType::Colour;
        WorldPickPassColour.source         = RenderTargetAttachmentSource::View; // Получить вложение из представления.
        WorldPickPassColour.LoadOperation  = RenderTargetAttachmentLoadOperation::DontCare;
        WorldPickPassColour.StoreOperation = RenderTargetAttachmentStoreOperation::Store;
        WorldPickPassColour.PresentAfter   = false;

        // Привязка глубины
        auto& WorldPickPassDepth = WorldPickPass.target.attachments[1];
        WorldPickPassDepth.type           = RenderTargetAttachmentType::Depth;
        WorldPickPassDepth.source         = RenderTargetAttachmentSource::View; // Получить вложение из представления.
        WorldPickPassDepth.LoadOperation  = RenderTargetAttachmentLoadOperation::DontCare;
        WorldPickPassDepth.StoreOperation = RenderTargetAttachmentStoreOperation::Store;
        WorldPickPassDepth.PresentAfter   = false;

        if (!RenderingSystem::RenderpassCreate(WorldPickPass, PickView.passes[0])) {
            MERROR("UI view — Не удалось создать проход рендеринга «%s»", PickView.passes[0].name.c_str());
            return false;
        };

        // UIPick pass
        // Конфигурация подпрохода рендеринга
        RenderpassConfig UiPickPass;
        UiPickPass.name                   = "Renderpass.Bultin.UiPick";
        UiPickPass.depth                  = 1.F;
        UiPickPass.stencil                = 0;
        UiPickPass.RenderArea             = FVec4(0.F, 0.F, (f32)config.StartWidth, (f32)config.StartHeight);  // Разрешение области рендеринга по умолчанию.
        UiPickPass.ClearColour            = FVec4(1.F, 1.F, 1.F, 1.F);
        UiPickPass.ClearFlags             = RenderpassClearFlag::None;
        UiPickPass.RenderTargetCount      = 1; // ПРИМЕЧАНИЕ: Тройная буферизация не применяется.
        UiPickPass.target.AttachmentCount = 1;
        UiPickPass.target.attachments     = (RenderTargetAttachmentConfig*)MemorySystem::Allocate(sizeof(RenderTargetAttachmentConfig) * UiPickPass.target.AttachmentCount, Memory::Array);
        
        // Привязка цвета
        auto& UiPickPassColour = UiPickPass.target.attachments[0];
        UiPickPassColour.type           = RenderTargetAttachmentType::Colour;
        UiPickPassColour.source         = RenderTargetAttachmentSource::View; // Получить вложение из представления.
        UiPickPassColour.LoadOperation  = RenderTargetAttachmentLoadOperation::Load;
        UiPickPassColour.StoreOperation = RenderTargetAttachmentStoreOperation::Store; // Необходимо сохранить его, чтобы впоследствии можно было взять образец.
        UiPickPassColour.PresentAfter   = false;

        if (!RenderingSystem::RenderpassCreate(UiPickPass, PickView.passes[1])) {
            MERROR("Pick view — Не удалось создать проход рендеринга «%s»", PickView.passes[1].name.c_str());
            return false;
        };

        PickView.OnRegistered               = RenderViewPick::OnRegistered;
        PickView.Destroy                    = RenderViewPick::Destroy;
        PickView.Resize                     = RenderViewPick::Resize;
        PickView.BuildPacket                = RenderViewPick::BuildPacket;
        PickView.Render                     = RenderViewPick::Render;
        PickView.RegenerateAttachmentTarget = RenderViewPick::RegenerateAttachmentTarget;
    }

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
    EventSystem::Register(EventSystem::DEBUG0,               &app, GameOnDebugEvent);
    EventSystem::Register(EventSystem::DEBUG1,               &app, GameOnDebugEvent);
    EventSystem::Register(EventSystem::DEBUG2,               &app, GameOnDebugEvent);
    EventSystem::Register(EventSystem::OojectHoverIdChanged, &app, GameOnEvent     );
    EventSystem::Register(EventSystem::ButtonReleased,  app.state, GameOnButton    );
    EventSystem::Register(EventSystem::MouseMoved,      app.state, GameOnMouseMove );
    EventSystem::Register(EventSystem::MouseDragBegin,  app.state, GameOnDrag      );
    EventSystem::Register(EventSystem::MouseDragEnd,    app.state, GameOnDrag      );
    EventSystem::Register(EventSystem::MouseDragged,    app.state, GameOnDrag      );
    //ЗАДАЧА: временно

    EventSystem::Register(EventSystem::KeyPressed,           &app, GameOnKey        );
    EventSystem::Register(EventSystem::KeyReleased,          &app, GameOnKey        );
    EventSystem::Register(EventSystem::MVarChanged,       nullptr, GameOnMVarChanged);
}

void ApplicationUnregisterEvents(Application &app)
{
    EventSystem::Unregister(EventSystem::DEBUG0,               &app, GameOnDebugEvent);
    EventSystem::Unregister(EventSystem::DEBUG1,               &app, GameOnDebugEvent);
    EventSystem::Unregister(EventSystem::DEBUG2,               &app, GameOnDebugEvent);
    EventSystem::Unregister(EventSystem::OojectHoverIdChanged, &app, GameOnEvent     );
    EventSystem::Unregister(EventSystem::ButtonReleased,  app.state, GameOnButton    );
    EventSystem::Unregister(EventSystem::MouseMoved,      app.state, GameOnMouseMove );
    EventSystem::Unregister(EventSystem::MouseDragBegin,  app.state, GameOnDrag      );
    EventSystem::Unregister(EventSystem::MouseDragEnd,    app.state, GameOnDrag      );
    EventSystem::Unregister(EventSystem::MouseDragged,    app.state, GameOnDrag      );

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
