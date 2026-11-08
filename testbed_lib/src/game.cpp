#include "game.h"

#include "core/input.h"
#include "core/metrics.h"

#include "systems/camera_system.hpp"
#include "systems/material_system.h"
#include "renderer/rendering_system.h"
#include "renderer/renderpass.h"
// #include <new>

#include <math/geometry_utils.h>

// Проходы отрисовки.
#include "passes/ui_pass.h"


// ЗАДАЧА: Реедактор временно
#include "editor/gizmo.h"

// ЗАДАЧА: временный код
#include "core/identifier.h"
#include "systems/geometry_system.h"
#include "systems/resource_system.h"
#include "systems/shader_system.h"
#include "views/render_view_pick.h"
#include "debug_console.h"
#include "elements/panel.h"
#include "elements/button.h"
// ЗАДАЧА: конец временного кода

/// @brief Закрытая структура, используемая для сортировки геометрии по расстоянию от камеры.
struct GeometryDistance {
    /// @brief Данные рендеринга геометрии.
    GeometryRenderData geometry;
    /// @brief Расстояние от камеры.
    f32 distance;
};

bool GameConfigureRenderViews(ApplicationConfig& config);
void ApplicationRegisterEvents(Application& app);
void ApplicationUnregisterEvents(Application& app);
static bool LoadMainScene(Application* app);
static bool CreateRendergraph(Application& app);

void Game::ClearDebugObjects() 
{
    if (TestBoxes) {
        const u32 BoxCount = TestBoxes.Length();
        for (u32 i = 0; i < BoxCount; ++i) {
            auto& box = TestBoxes[i];
            box.Unload();
            box.Destroy();
        }
        TestBoxes.Clear();
    }

    if (TestLines) {
        const u32 LineCount = TestLines.Length();
        for (u32 i = 0; i < LineCount; ++i) {
            auto line = TestLines[i];
            line.Unload();
            line.Destroy();
        }
        TestLines.Clear();
    }
}

static bool GameOnEvent(u16 code, void *sender, void *ListenerInst, const EventContext& context)
{
    auto GameInst = (Application*)ListenerInst;
    auto state = (Game*)GameInst->state;
    switch (code) {
        case EventSystem::OojectHoverIdChanged: {
            state->HoveredObjectID = context.data.u32[0];
            return true;
        }
        case EventSystem::SetRenderMode: {
            i32 mode = context.data.i32[0];
            switch (mode) {
                default:
                case Render::DebugViewMode::Default:
                    MDEBUG("Режим рендеринга установлен по умолчанию.");
                    state->RenderMode = Render::DebugViewMode::Default;
                    break;
                case Render::DebugViewMode::Lighting:
                    MDEBUG("Режим рендеринга установлен на освещение.");
                    state->RenderMode = Render::DebugViewMode::Lighting;
                    break;
                case Render::DebugViewMode::Normals:
                    MDEBUG("Режим рендеринга установлен на нормали.");
                    state->RenderMode = Render::DebugViewMode::Normals;
                    break;
                case Render::DebugViewMode::Cascades:
                    MDEBUG("Режим рендеринга установлен на каскадный.");
                    state->RenderMode = Render::DebugViewMode::Cascades;
                    break;
            }
            return true;
        }
    }

    return false;
}

// ЗАДАЧА: временно
static bool GameOnDebugEvent(u16 code, void *sender, void *ListenerInst, const EventContext& context)
{
    auto GameInst = (Application*)ListenerInst;
    auto state = (Game*)GameInst->state;
    switch (code) {
    case EventSystem::DEBUG0: {
        static constexpr const char* names[3] = {
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
    }
    case EventSystem::DEBUG1: {
        if (state->MainScene.state < SimpleScene::State::Loading) {
            MDEBUG("Загрузка основной сцены...");
            
            if (!LoadMainScene(GameInst)) {
                MERROR("Не удалось загрузить основную сцену!");
            }
        }
        return true;
    }
    case EventSystem::DEBUG2: {
        if (state->MainScene.state == SimpleScene::State::Loaded) {
            MDEBUG("Выгрузка сцены...");
            state->MainScene.Unload(false);
            state->ClearDebugObjects();
            MDEBUG("Выполнено.");
        }
        
        return true;
    }
    case EventSystem::DEBUG3: {

    }
    default:
        return false;
    }
}
//ЗАДАЧА: временно

static bool GameOnDrag(u16 code, void* sender, void* ListenerInst, const EventContext& context)
{
    i16 x = context.data.i16[0];
    i16 y = context.data.i16[1];
    Buttons DragButton = static_cast<Buttons>(context.data.u16[2]);
    auto state = (Game*)ListenerInst;

    // Обращайте внимание только на перетаскивание левой кнопкой.
    if (DragButton == Buttons::Left) {
        auto view = state->WorldCamera->GetView();
        auto origin = state->WorldCamera->GetPosition();

        auto& viewport = state->WorldViewport;

        Ray ray {
            FVec2((f32)x, (f32)y),
            viewport.rect,
            origin,
            view,
            viewport.projection
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

static bool GameOnButton(u16 code, void* sender, void* ListenerInst, const EventContext& context) {
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

                auto& viewport = state->WorldViewport;
                // Разрешить это действие только в «основном» окне просмотра.
                if (Math::PointInRect2d(Point(x,y), viewport.rect)) {
                    Ray ray {
                        FVec2((f32)x, (f32)y),
                        viewport.rect,
                        origin,
                        view,
                        viewport.projection
                    };

                    RaycastResult RayResult;
                    if (state->MainScene.Raycast(ray, RayResult)) {
                        const u32 HitCount = RayResult.hits.Length();
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
                            MINFO("Выбор объекта отменен.");
                            state->selection.xform = nullptr;
                            state->selection.UniqueID = INVALID::ID;

                            state->gizmo.SetSelectedTransform();
                        }

                        // ЗАДАЧА: скрыть гаджет, отключить ввод и т. д.
                    }
                }
                
                

            } break;
            default: break;
        }
    }

    return false;
}

static bool GameOnMouseMove(u16 code, void* sender, void* ListenerInst, const EventContext& context) {
    if (code == EventSystem::MouseMoved && !InputSystem::IsButtonDragging(Buttons::Left)) {
        i16 x = context.data.i16[0];
        i16 y = context.data.i16[1];

        auto state = (Game*)ListenerInst;

        const auto& view = state->WorldCamera->GetView();
        const auto& origin = state->WorldCamera->GetPosition();

        auto& viewport = state->WorldViewport;

        Ray ray {
            FVec2((f32)x, (f32)y),
            viewport.rect,
            origin,
            view,
            viewport.projection
        };

        state->gizmo.HandleInteraction(state->WorldCamera, ray, Gizmo::InteractionType::MouseHover);
    }
    return false;  // Разрешить другим обработчикам событий получать это событие.
}

bool ApplicationBoot(Application& app)
{
    MINFO("Загрузка испытательной сборки...");

    app.state = MemorySystem::Allocate(sizeof(Game), Memory::Game, true);
    // app.state = new(app.state) Game();

    app.AppConfig.AppFrameDataSize = sizeof(TestbedApplicationFrameData);

    app.AppConfig.FontConfig.AutoRelease = false;
    app.AppConfig.FontConfig.DefaultBitmapFontCount = 1;

    // BitmapFontConfig BmpFontConfig { "Metrika 21px", 21, "Metrika" };
    app.AppConfig.FontConfig.BitmapFontConfigs = new BitmapFontConfig("Metrika 21px", 21, "Metrika");

    // SystemFontConfig SysFontConfig { "Noto Sans", 20, "NotoSansCJK" };
    app.AppConfig.FontConfig.DefaultSystemFontCount = 1;
    app.AppConfig.FontConfig.SystemFontConfigs = new SystemFontConfig("Noto Sans", 20, "NotoSansCJK");

    app.AppConfig.FontConfig.MaxBitmapFontCount = app.AppConfig.FontConfig.MaxSystemFontCount = 101;

    if (!CreateRendergraph(app)) {
        MERROR("Не удалось создать графы рендеринга. Завершение работы приложения.");
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
    
    auto state = (Game*)app.state;

    #pragma region Инициализация графов

    if (!state->ForwardGraph.Initialize()) {
        MERROR("Не удалось загрузить ресурсы графа рендеринга.");
        return false;
    }

    if (!state->EditorGraph.Initialize()) {
        MERROR("Не удалось загрузить ресурсы графа рендеринга редактора."); 
        return false;
    }
    
    if (!state->UiGraph.Initialize()) {
        MERROR("Не удалось загрузить ресурсы графа отрисовки стандартного пользовательского интерфейса.");
        return false;
    }

    #pragma endregion // Инициализация графов

    #pragma region Инициализация системы пользовательского интерфейса
    auto& sysManager = app.engine->GetSystemsManager();
    UiSystem::Config uiCfg = { 1024 };
    u64 size;
    UiSystem::Initialize(size, nullptr, uiCfg);
    auto uisys = (UiSystem*)sysManager.GetMem(size);
    UiSystem::Initialize(size, uisys, uiCfg);
    if (!sysManager.Register(M_SYSTEM_TYPE_STANDARD_UI_EXT, uisys, UiSystem::Update, UiSystem::Shutdown, UiSystem::RenderPrepareFrame)) {
        MERROR("Не удалось зарегистрировать стандартную систему пользовательского интерфейса.");
        return false;
    }
    #pragma endregion

    ApplicationRegisterEvents(app);
    ResourceSystem::RegisterLoader(eResource::Type::SimpleScene, MString(), "scenes");

    state->selection.UniqueID = INVALID::ID;
    state->selection.xform = nullptr;
    state->console.Load();

        // Настройка области просмотра.
        // Область просмотра мира
        if (!state->WorldViewport.Create(Rect2D(20, 20, 1280 - 40, 720 - 40), Math::DegToRad(45.F), 0.1F, 1000.F, RendererProjectionMatrixType::Perspective)) {
            MERROR("Не удалось создать область просмотра мира. Не удаётся запустить приложение.");
            return false;
        }

        // Область просмотра пользовательского интерфейса
        if (!state->UiViewport.Create(Rect2D(0, 0, 1280, 720), 0.F, -100.F, 100.F, RendererProjectionMatrixType::Orthographic)) {
            MERROR("Не удалось создать область просмотра пользовательского интерфейса. Не удаётся запустить приложение.");
            return false;
        }

        // ЗАДАЧА: тестирование
        if (!state->WorldViewport2.Create(Rect2D(20, 20, 1280 - 40, 720 - 40), Math::DegToRad(45.F), 0.01F, 10.F, RendererProjectionMatrixType::OrthographicCentered)) {
            MERROR("Не удалось создать область просмотра 2. Не удается запустить приложение.");
            return false;
        }

    state->ForwardMoveSpeed  =  5.F * 5.F;
    state->BackwardMoveSpeed = 2.5F * 5.F;

    if (!state->gizmo.Initialize()) {
        MERROR("Не удалось инициализировать редактор!");
        return false;
    }

    if (!state->gizmo.Load()) {
        MERROR("Не удалось загрузить редактор!");
        return false;
    }
    state->EditorGraph.SetGizmo(state->gizmo);

    // Сетки мира
    // Отменить все сетки.
    for (u32 i = 0; i < 10; ++i) {
        state->meshes[i].generation = INVALID::U8ID;
        state->UiMeshes[i].generation = INVALID::U8ID;
    }

    state->TestText = Label("testbed_metrika_test_text", FontType::Bitmap, "Metrika 21px", 21, "Какой-то тестовый текст 123,\n\tyo!");
    if (!state->TestText.Load()) {
        MERROR("Не удалось загрузить текст тестовой системы.");
    } else {
        if (!uisys->RegisterControl(&state->TestText)) {
            MERROR("Не удалось зарегистрировать элемент управления.");
        } else {
            if (!uisys->AddChild(&state->TestText)) {
                MERROR("Не удалось установить родительский текст тестовой системы.");
            } else {
                state->TestText.IsActive = true;
            }
        }
    }

    // Переместить отладочный текст в новую нижнюю часть экрана.
    state->TestText.SetPosition(FVec3(20.F, state->height - 75.F, 0.F)); 

    #pragma region Стандартный интерфейс.
    Panel* TestPanel = nullptr;
    if (!(TestPanel = state->TestPanel = new Panel("test_panel", FVec2(300.F, 300.F), FVec4(0.F, 0.F, 0.F, 0.5F)))) {
        MERROR("Не удалось создать тестовую панель.");
    } else {
        if (!TestPanel->Load()) {
            MERROR("Не удалось загрузить тестовую панель.");
        } else {
            TestPanel->xform.Translate(FVec3(950.F, 350.F));
            auto uiSystem = (UiSystem*)SystemsManager::GetState(M_SYSTEM_TYPE_STANDARD_UI_EXT);
            if (!uiSystem->RegisterControl(TestPanel)) {
                MERROR("Не удалось зарегистрировать элемент управления.");
            } else {
                if (!uiSystem->AddChild(TestPanel)) {
                    MERROR("Не удалось создать родительскую тестовую панель.");
                } else {
                    TestPanel->IsActive = true;
                }
            }
        }
    }

    state->TestButton = Button("test_button");
    // Назначаем обработчик щелчков.
    state->TestButton.OnClick = [](UiElement *self, UiMouseEvent event) {
        if (self) {
            MDEBUG("Нажал '%s'!", (S_Arg(self->name)));
        }
    };

    // Переместите и поверните его немного.
    // Quaternion rotation {FVec3(0.F, 0.F, 1.F), Math::DegToRad(-45.F), false};
    // state->TestButton->xform.TranslateRotate(FVec3(50.F, 50.F, 0.F), rotation);

    if (!state->TestButton.Load()) {
        MERROR("Не удалось загрузить тестовую кнопку.");
    } else {
        if (!uisys->RegisterControl(&state->TestButton)) {
            MERROR("Не удалось зарегистрировать элемент управления.");
        } else {
            if (!TestPanel->AddChild(&state->TestButton)) {
                MERROR("Не удалось создать родительскую тестовую кнопку.");
            } else {
                state->TestButton.IsActive = true;
            }
        }
    }
    state->TestButton.SetPosition(5.F, 5.F);

    state->TestSysText = Label("testbed_UTF_test_text", FontType::System, "Noto Sans CJK JP", 31, "Нажмите 'L', чтобы загрузить \n\tсцену!\n\n\tyo!\n\n\tこんにちは 한");
    if (!state->TestSysText.Load()) {
        MERROR("Не удалось загрузить системный текст для тестирования.");
    } else {
        if (!uisys->RegisterControl(&state->TestSysText)) {
            MERROR("Не удалось зарегистрировать элемент управления.");
        } else {
            if (!TestPanel->AddChild(&state->TestSysText)) {
                MERROR("Не удалось установить родительский текст тестовой системы.");
            } else {
                state->TestSysText.IsActive = true;
            }
        }
    }

    state->TestSysText.SetPosition(5.F, 100.F);
    #pragma endregion // Стандартный интерфейс

    FVec3 CamPos { 5.83F, 4.35F, 18.68F };
    FVec3 CamRot { -29.43F, -42.41F, 0.F };
    
    state->WorldCamera = CameraSystem::Acquire("world");
    state->WorldCamera->SetPosition(CamPos);
    state->WorldCamera->SetRotationEuler(CamRot);

    state->WorldCamera2 = CameraSystem::Acquire("world2");
    state->WorldCamera2->SetPosition(CamPos);
    state->WorldCamera2->SetRotationEuler(CamRot);

    state->UpdateClock.Zero();
    state->PrepareClock.Zero();
    state->RenderClock.Zero();
    
    return state->running = true;
}

bool ApplicationUpdate(Application& app, const FrameData& rFrameData)
{
    // auto AppFrameData = reinterpret_cast<TestbedApplicationFrameData*>(rFrameData.ApplicationFrameData);
    auto state = (Game*)app.state;

    if (!state->running) {
        return true;
    }

    state->UpdateClock.Start();

    // ЗАДАЧА: тестирование изменения размера
    static f32 ButtonHeight = 50.F;
    ButtonHeight = 50.F + (Math::sin(rFrameData.TotalTime) * 20.F);
    state->TestButton.SetHeight(ButtonHeight);

    // Обновите растровый текст с учётом позиции камеры. ПРИМЕЧАНИЕ: пока используется только камера по умолчанию.
    FVec3 pos = state->WorldCamera->GetPosition();
    FVec3 rot = state->WorldCamera->GetRotationEuler();

    Viewport& ViewViewport = state->WorldViewport;

    f32 NearClip = ViewViewport.NearClip;
    f32 FarClip = ViewViewport.FarClip;

    SimpleScene& scene = state->MainScene;
    if (scene.state >= SimpleScene::State::Loaded) {
        if (!scene.Update(rFrameData)) {
            MWARN("Не удалось обновить основную сцену.");
        }
        // Обновление уровней детализации (LOD) для сцены в зависимости от расстояния до камеры.
        scene.UpdateLODfromViewPosition(pos, NearClip, FarClip, rFrameData);

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
                MCLAMP(Math::sin(rFrameData.TotalTime) * 75.F + 50.F, 0.F, 100.F),
                MCLAMP(Math::sin(rFrameData.TotalTime + (M_2PI / 3)) * 75.F + 50.F, 0.F, 100.F),
                MCLAMP(Math::sin(rFrameData.TotalTime + (M_4PI / 3)) * 75.F + 50.F, 0.F, 100.F),
                1.F); 
            state->PointLight1->data.position.z = 20.F + Math::sin(rFrameData.TotalTime);
        }
    }

    // Отслеживайте различия в распределении.
    state->PrevAllocCount = state->AllocCount;
    state->AllocCount = MemorySystem::GetMemoryAllocCount();

    // Также прикрепите текущее состояние мыши.
    bool LeftDown = InputSystem::IsButtonDown(Buttons::Left);
    bool RightDown = InputSystem::IsButtonDown(Buttons::Right);
    i16 MouseX, MouseY;
    InputSystem::GetMousePosition(MouseX, MouseY);

    // Преобразовать в NDC
    f32 mouseXNdc = Math::RangeConvertF32((f32)MouseX, 0.F, (f32)state->width,  -1.F, 1.F);
    f32 MouseYNdc = Math::RangeConvertF32((f32)MouseY, 0.F, (f32)state->height, -1.F, 1.F);

    f64 fps, FrameTime;
    auto metrics = Metrics::GetInstance();
    metrics->Frame(fps, FrameTime);

    // Сохраняйте скользящее среднее значение таймеров обновления и рендеринга за последнюю ~1 секунду.
    static f64 AccumulatedMs = 0;
    static f32 TotalUpdateSeconds = 0;
    static f32 TotalPrepareSeconds = 0;
    static f32 TotalRenderSeconds = 0;

    static f32 TotalUpdateAvgUs = 0;
    static f32 TotalPrepareAvgUs = 0;
    static f32 TotalRenderAvgUs = 0;
    static f32 TotalAvg = 0;  // Общее среднее значение по кадру

    TotalUpdateSeconds += state->LastUpdateElapsed;
    TotalPrepareSeconds += state->PrepareClock.elapsed;
    TotalRenderSeconds += state->RenderClock.elapsed;
    AccumulatedMs += FrameTime;

    // По прошествии примерно 1 секунды вычислите среднее значение и очистите аккумуляторы.
    if (AccumulatedMs >= 1000.F) {
        TotalUpdateAvgUs = (TotalUpdateSeconds / AccumulatedMs) * M_SEC_TO_US_MULTIPLIER;
        TotalPrepareAvgUs = (TotalPrepareSeconds / AccumulatedMs) * M_SEC_TO_US_MULTIPLIER;
        TotalRenderAvgUs = (TotalRenderSeconds / AccumulatedMs) * M_SEC_TO_US_MULTIPLIER;
        TotalAvg = TotalUpdateAvgUs + TotalPrepareAvgUs + TotalRenderAvgUs;
        TotalRenderSeconds = 0;
        TotalPrepareSeconds = 0;
        TotalUpdateSeconds = 0;
        AccumulatedMs = 0;
    }

    auto rSystem = SystemsManager::GetRenderingSystem();
    const char* VsyncText = rSystem->FlagEnabled(RenderingConfigFlagBits::VsyncEnabledBit) ? "Вкл" : "Выкл";
    char TextBuffer[2048]{};
    MString::Format(
        TextBuffer,
        "\
        FPS: %5.1f(%4.1fмс) Позиция=[%7.3F, %7.3F, %7.3F] Вращение=[%7.3F, %7.3F, %7.3F]\n\
        Upd: %8.3fмкс, Подг: %8.3fмкс, Rend: %8.3fмкс, Tot: %8.3fмкс \n\
        Мышь: X=%-5d Y=%-5d   L=%s R=%s   NDC: X=%.6f, Y=%.6f\n\
        Vsync: %s Draw: %-5u (%-5u shadow pass) Hovered: %s%u",
        fps,
        FrameTime,
        pos.x, pos.y, pos.z,
        Math::RadToDeg(rot.x), Math::RadToDeg(rot.y), Math::RadToDeg(rot.z),
        TotalUpdateAvgUs,
        TotalPrepareAvgUs,
        TotalRenderAvgUs,
        TotalAvg,
        MouseX, MouseY,
        LeftDown ? "Y" : "N",
        RightDown ? "Y" : "N",
        mouseXNdc,
        MouseYNdc,
        VsyncText,
        rFrameData.DrawnMeshCount,
        rFrameData.DrawnShadowMeshCount,
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

bool ApplicationPrepareFrame(Application& app, FrameData& rFrameData) 
{
    auto state = (Game*)app.state;

    if (!state->running) {
        return false;
    }

    state->PrepareClock.Start();

    #pragma region 

    if (!state->ForwardGraph.FramePrepare(rFrameData, *state->WorldCamera, state->WorldViewport, state->MainScene, state->RenderMode)) {
        MERROR("Не удалось подготовить данные кадра для прямого рендеринга.");
        return false;
    }

    if (!state->EditorGraph.FramePrepare(rFrameData, *state->WorldCamera, state->WorldViewport, state->MainScene, state->RenderMode)) {
        MERROR("Не удалось подготовить данные кадра для рендеринга редактора.");
        return false;
    }

    if (!state->UiGraph.FramePrepare(rFrameData, *state->WorldCamera, state->WorldViewport, state->MainScene, state->RenderMode)) {
        MERROR("Не удалось подготовить данные кадра для стандартного рендеринга пользовательского интерфейса.");
        return false;
    }

    state->PrepareClock.Update();
    
    return true;
}

bool ApplicationRenderFrame(Application& app, FrameData& rFrameData)
{
    auto state = (Game*)app.state;
    if (!state->running) {
        return true;
    }

    state->RenderClock.Start();

    if (!state->ForwardGraph.ExecuteFrame(rFrameData)) {
        MERROR("Не удалось выполнить кадр в графе прямого рендеринга. Подробности см. в журналах.");
        return false;
    }

    if (!state->EditorGraph.ExecuteFrame(rFrameData)) {
        MERROR("Не удалось выполнить кадр в графе рендеринга редактора. Подробности см. в журналах.");
        return false;
    }

    if (!state->UiGraph.ExecuteFrame(rFrameData)) {
        MERROR("Не удалось выполнить кадр в стандартном графе рендеринга пользовательского интерфейса. Подробности см. в журналах.");
        return false;
    }
    
    state->RenderClock.Update();

    return true;
}

void ApplicationOnResize(Application& app, u32 width, u32 height) 
{
    if (!app.state || !width || !height) {
        return;
    }
    
    auto state = (Game*)app.state;

    state->width = width;
    state->height = height;

    // i32 HalfWidth = state->width * 0.5F;

    // Изменение размера областей просмотра.
    // Область просмотра мира — правая сторона
    Rect2D vw { 0, 0, state->width, state->height };
    state->WorldViewport.Resize(vw); // {HalfWidth + 20, 20, HalfWidth - 40, state->height - 40};

    // Область просмотра интерфейса
    state->UiViewport.Resize(vw);

    // Область просмотра мира 2
    state->WorldViewport2.Resize(vw); // Rect2D(20, 20, HalfWidth - 40, state->height - 40)

    // ЗАДАЧА: временный блок кода
    // Переместить отладочный текст в новую нижнюю часть экрана.
    state->TestText.SetPosition(FVec3(20, state->height - 105, 0));
    FVec2 TestPanelSize = state->TestPanel->Size();
    f32 x = state->width - TestPanelSize.x - 30;
    f32 y = state->height - TestPanelSize.y - 30;
    state->TestPanel->SetPosition(x, y);
    // ЗАДАЧА: конец временного блока кода

    // Передать изменение размера на рендерграф.
    if (!state->ForwardGraph.OnResize(width, height)) {
        MERROR("Ошибка изменения размера графа отрисовки. Подробности см. в логах.");
    }

    if (!state->EditorGraph.OnResize(width, height)) {
        MERROR("Ошибка изменения размера графа отрисовки редактора. Подробности см. в логах.");
    }

    if (!state->UiGraph.OnResize(width, height)) {
        MERROR("Ошибка изменения размера графа отрисовки стандартного пользовательского интерфейса. Подробности см. в логах.");
    }
}

void ApplicationShutdown(Application& app)
{
    auto state = (Game*)app.state;
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
    state->ForwardGraph.Destroy();
    state->EditorGraph.Destroy();
    state->UiGraph.Destroy();
}

void ApplicationLibOnLoad(Application& app)
{
    ApplicationRegisterEvents(app);
    ((Game*)app.state)->console.OnLibLoad(app.stage >= Application::Stage::BootComplete);
    if (app.stage >= Application::Stage::BootComplete) {
        GameSetupCommands();
        GameSetupKeymaps(&app);
    }
}

void ApplicationLibOnUnload(Application& app)
{
    ApplicationUnregisterEvents(app);
    ((Game*)app.state)->console.OnLibUnload();
    GameRemoveCommands();
    GameRemoveKeymaps(&app);
}

bool GameConfigureRenderViews(ApplicationConfig& config)
{
    // ЗАДАЧА: Разделить на 2 представления и снова включить.
    // Pick view
    /*{
        auto& PickView = config.RenderViews[Testbed::Pick];
        PickView.name = "pick";
        // PickView.width = 0;
        // PickView.height = 0;
        PickView.RenderpassCount = 2;
        PickView.passes = (Renderpass*)MemorySystem::Allocate(sizeof(Renderpass) * PickView.RenderpassCount, Memory::Array, true);

        RenderpassConfig WorldPickPass;
        WorldPickPass.name                   = "Renderpass.Builtin.WorldPick";
        WorldPickPass.depth                  = 1.F;
        WorldPickPass.stencil                = 0;
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
    }*/

    return true;
}

static void ToggleVsync() {
    auto renderer = SystemsManager::GetRenderingSystem();
    bool VsyncEnabled = renderer->FlagEnabled(RenderingConfigFlagBits::VsyncEnabledBit);
    VsyncEnabled = !VsyncEnabled;
    renderer->FlagSetEnabled(RenderingConfigFlagBits::VsyncEnabledBit, VsyncEnabled);
}

static bool GameOnMVarChanged(u16 code, void* sender, void* ListenerInst, const EventContext& data) {
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
    EventSystem::Register(EventSystem::DEBUG3,               &app, GameOnDebugEvent);
    EventSystem::Register(EventSystem::OojectHoverIdChanged, &app, GameOnEvent     );
    EventSystem::Register(EventSystem::SetRenderMode,        &app, GameOnEvent     );
    EventSystem::Register(EventSystem::ButtonReleased,  app.state, GameOnButton    );
    EventSystem::Register(EventSystem::MouseMoved,      app.state, GameOnMouseMove );
    EventSystem::Register(EventSystem::MouseDragBegin,  app.state, GameOnDrag      );
    EventSystem::Register(EventSystem::MouseDragEnd,    app.state, GameOnDrag      );
    EventSystem::Register(EventSystem::MouseDragged,    app.state, GameOnDrag      );
    //ЗАДАЧА: временно

    EventSystem::Register(EventSystem::MVarChanged,       nullptr, GameOnMVarChanged);
}

void ApplicationUnregisterEvents(Application &app)
{
    EventSystem::Unregister(EventSystem::DEBUG0,               &app, GameOnDebugEvent);
    EventSystem::Unregister(EventSystem::DEBUG1,               &app, GameOnDebugEvent);
    EventSystem::Unregister(EventSystem::DEBUG2,               &app, GameOnDebugEvent);
    EventSystem::Unregister(EventSystem::DEBUG3,               &app, GameOnDebugEvent);
    EventSystem::Unregister(EventSystem::OojectHoverIdChanged, &app, GameOnEvent     );
    EventSystem::Unregister(EventSystem::SetRenderMode,        &app, GameOnEvent     );
    EventSystem::Unregister(EventSystem::ButtonReleased,  app.state, GameOnButton    );
    EventSystem::Unregister(EventSystem::MouseMoved,      app.state, GameOnMouseMove );
    EventSystem::Unregister(EventSystem::MouseDragBegin,  app.state, GameOnDrag      );
    EventSystem::Unregister(EventSystem::MouseDragEnd,    app.state, GameOnDrag      );
    EventSystem::Unregister(EventSystem::MouseDragged,    app.state, GameOnDrag      );

    // ЗАДАЧА: конец временного блока кода

    EventSystem::Unregister(EventSystem::MVarChanged, nullptr, GameOnMVarChanged);
}

bool LoadMainScene(Application *app)
{
    auto state = (Game*)app->state;

    // Загрузить файл конфигурации
    // ЗАДАЧА: очистить ресурс.
    SimpleSceneResource simpleSceneResource;
    if (!ResourceSystem::Load("test_scene", nullptr, simpleSceneResource)) {
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

bool CreateRendergraph(Application &app)
{
    auto state = (Game*)app.state;

    bool result = false;

    ForwardRendergraphConfig fConfig { .ShadowmapResolution = 2048 };
    state->ForwardGraph = ForwardRendergraph(result, &fConfig);
    if (!result) {
        MERROR("Не удалось инициализировать граф прямого рендеринга.");
        return false;
    }

    EditorRendergraphConfig eConfig { .dummy = 0 };
    state->EditorGraph = EditorRendergraph(result, &eConfig);
    if (!result) {
        MERROR("Не удалось инициализировать граф рендеринга редактора.");
        return false;
    }

    UiRendergraphConfig uiConfig { .dummy = 0 };
    state->UiGraph = UiRendergraph(result, &uiConfig);
    if (!result) {
        MERROR("Не удалось инициализировать граф отрисовки пользовательского интерфейса.");
        return false;
    }

    return true;
}
