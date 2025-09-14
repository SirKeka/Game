#pragma once

#include <application_types.h>
#include <math/frustrum.h>
#include "editor/gizmo.h"
#include <resources/simple_scene.h>

//ЗАДАЧА: Временно
#include <core/keymap.h>
#include <resources/skybox.h>

#include <systems/light_system.h>
#include "resources/simple_scene.h"

#include "resources/debug/debug_box3d.h"
#include "resources/debug/debug_line3d.h"

#include "debug_console.h"

struct Game
{
    bool running;
    class Camera* WorldCamera;

    u16 width, height;

    Frustum CameraFrustrum;

    Clock UpdateClock;
    Clock RenderClock;
    f64 LastUpdateElapsed;

    // ЗАДАЧА: временно
    SimpleScene MainScene;
    bool MainSceneUnloadTriggered;
    Mesh meshes[10];

    PointLight* PointLight1;

    Mesh UiMeshes[10];
    Text TestText;
    Text TestSysText;

    DebugConsole console;

    // Уникальный идентификатор текущего объекта, на который наведен курсор.
    u32 HoveredObjectID;

    Keymap ConsoleKeymap;

    u64 AllocCount;
    u64 PrevAllocCount;

    f32 ForwardMoveSpeed;
    f32 BackwardMoveSpeed;

    Gizmo gizmo;

    // Используется для визуализации наших кастов/столкновений.
    DArray<DebugLine3D> TestLines;
    DArray<DebugBox3D> TestBoxes;
    // ЗАДАЧА: конец временно
    // constexpr Game(const ApplicationConfig& config) : Application(config, sizeof(State)) {}

    void ClearDebugObjects();
};

extern "C" MAPI bool ApplicationBoot(Application& app);

/// @brief Функция инициализации
/// @return true в случае успеха; в противном случае false.
extern "C" MAPI bool ApplicationInitialize(Application& app);
/// @brief Функция обновления игры
/// @param DeltaTime Время в секундах с момента последнего кадра.
/// @return true в случае успеха; в противном случае false.
extern "C" MAPI bool ApplicationUpdate(Application& app, const FrameData& rFrameData);
/// @brief Функция рендеринга игры
/// @param DeltaTime Время в секундах с момента последнего кадра.
/// @return true в случае успеха; в противном случае false.
extern "C" MAPI bool ApplicationRender(Application& app, RenderPacket& packet, FrameData& rFrameData);
/// @brief Функция изменения размера окна игры
/// @param Width ширина окна в пикселях.
/// @param Height высота окна в пикселях.
extern "C" MAPI void ApplicationOnResize(Application& app, u32 width, u32 height);

/// @brief Завершает игру, вызывая высвобождение ресурсов.
extern "C" MAPI void ApplicationShutdown(Application& app);

extern "C" MAPI void ApplicationLibOnLoad(Application& app);
extern "C" MAPI void ApplicationLibOnUnload(Application& app);

void GameSetupCommands();
void GameRemoveCommands();
void GameSetupKeymaps(Application* app);
void GameRemoveKeymaps(Application* app);

struct TestbedApplicationFrameData {
    DArray<GeometryRenderData> WorldGeometries{};
};

namespace Testbed
{
    enum PacketViews {
        Skybox,
        World,
        EditorWorld,
        UI,
        Pick,
        Max = Pick + 1
    };
} // namespace Testbed
