#pragma once

#include <application_types.hpp>
#include <math/frustrum.hpp>
#include <core/keymap.hpp>
#include <resources/skybox.hpp>
#include "debug_console.hpp"
#include <systems/light_system.hpp>

struct Game
{
    f32 DeltaTime;
    class Camera* WorldCamera;

    u16 width, height;

    Frustrum CameraFrustrum;

    Clock UpdateClock;
    Clock RenderClock;
    f64 LastUpdateElapsed;

    // ЗАДАЧА: временно
    Skybox sb;

    Mesh meshes[10];
    Mesh* CarMesh;
    Mesh* SponzaMesh;
    bool ModelsLoaded;

    DirectionalLight DirLight;
    PointLight PointLights[3];

    Mesh UiMeshes[10];
    Text TestText;
    Text TestSysText;

    DebugConsole console;

    // Уникальный идентификатор текущего объекта, на который наведен курсор.
    u32 HoveredObjectID;

    Keymap ConsoleKeymap;

    u64 AllocCount;
    u64 PrevAllocCount;
    // ЗАДАЧА: конец временно
    // constexpr Game(const ApplicationConfig& config) : Application(config, sizeof(State)) {}
};

    extern "C" MAPI bool ApplicationBoot(Application& app);

    /// @brief Функция инициализации
    /// @return true в случае успеха; в противном случае false.
    extern "C" MAPI bool ApplicationInitialize(Application& app);
    /// @brief Функция обновления игры
    /// @param DeltaTime Время в секундах с момента последнего кадра.
    /// @return true в случае успеха; в противном случае false.
    extern "C" MAPI bool ApplicationUpdate(Application& app, f32 DeltaTime);
    /// @brief Функция рендеринга игры
    /// @param DeltaTime Время в секундах с момента последнего кадра.
    /// @return true в случае успеха; в противном случае false.
    extern "C" MAPI bool ApplicationRender(Application& app, RenderPacket& packet, f32 DeltaTime);
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
