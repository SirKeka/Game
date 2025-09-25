#pragma once

#include <application_types.h>
#include <math/frustrum.h>
#include "editor/gizmo.h"
#include "renderer/viewport.h"

//ЗАДАЧА: Временно
#include <core/keymap.h>
#include <resources/skybox.h>

#include <systems/light_system.h>
#include "resources/simple_scene.h"

#include "resources/debug/debug_box3d.h"
#include "resources/debug/debug_line3d.h"

#include "debug_console.h"

class Camera;

struct SelectedObject {
    u32 UniqueID;
    Transform* xform;
};

struct Game
{
    bool running;
    Camera* WorldCamera;

    Camera* WorldCamera2;

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

    Viewport WorldViewport;
    Viewport UiViewport;

    Viewport WorldViewport2;

    SelectedObject selection;
    bool UsingGizmo;
    // ЗАДАЧА: конец временно
    // constexpr Game(const ApplicationConfig& config) : Application(config, sizeof(State)) {}

    void ClearDebugObjects();
};

/// @param app ссылка на приложение
extern "C" MAPI bool ApplicationBoot(Application& app);

/// @brief Функция инициализации
/// @return true в случае успеха; в противном случае false.
extern "C" MAPI bool ApplicationInitialize(Application& app);

/// @brief Функция обновления игры
/// @param app ссылка на приложение
/// @param rFrameData ссылка на данные кадра
/// @return true в случае успеха; в противном случае false.
extern "C" MAPI bool ApplicationUpdate(Application& app, const FrameData& rFrameData);

/// @brief 
/// @param app 
/// @param packet 
/// @param rFrameData ссылка на данные кадра
/// @return 
extern "C" MAPI bool ApplicationPrepareRenderPacket(Application& app, RenderPacket& packet, FrameData& rFrameData);
/// @brief Функция рендеринга игры
/// @param app ссылка на приложение
/// @param packet
/// @param rFrameData ссылка на данные кадра
/// @return true в случае успеха; в противном случае false.
extern "C" MAPI bool ApplicationRender(Application& app, RenderPacket& packet, FrameData& rFrameData);

/// @brief Функция изменения размера окна игры
/// @param app ссылка на приложение
/// @param Width ширина окна в пикселях.
/// @param Height высота окна в пикселях.
extern "C" MAPI void ApplicationOnResize(Application& app, u32 width, u32 height);

/// @brief Завершает игру, вызывая высвобождение ресурсов.
/// @param app ссылка на приложение
extern "C" MAPI void ApplicationShutdown(Application& app);

/// @param app ссылка на приложение
extern "C" MAPI void ApplicationLibOnLoad(Application& app);

/// @param app ссылка на приложение
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
        World,
        EditorWorld,
        UI,
        Pick,
        Max = Pick + 1
    };
} // namespace Testbed
