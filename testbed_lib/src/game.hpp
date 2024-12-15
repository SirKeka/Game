#pragma once

#include <application_types.hpp>
#include <math/frustrum.hpp>
#include <core/keymap.hpp>
#include <resources/skybox.hpp>
<<<<<<< Updated upstream

class Game : public Application
{
public:
    struct State{
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

        Mesh UiMeshes[10];
        Text TestText;
        Text TestSysText;

        // Уникальный идентификатор текущего объекта, на который наведен курсор.
        u32 HoveredObjectID;

        Keymap ConsoleKeymap;

        u64 AllocCount;
        u64 PrevAllocCount;
        // ЗАДАЧА: конец временно
    };
    
public:
    constexpr Game(const ApplicationConfig& config) : Application(config, sizeof(State)) {}
    ~Game();

    bool Boot() override;

    /// @brief Функция инициализации
    /// @return true в случае успеха; в противном случае false.
    bool Initialize() override;
    /// @brief Функция обновления игры
    /// @param DeltaTime Время в секундах с момента последнего кадра.
    /// @return true в случае успеха; в противном случае false.
    bool Update(f32 DeltaTime) override;
    /// @brief Функция рендеринга игры
    /// @param DeltaTime Время в секундах с момента последнего кадра.
    /// @return true в случае успеха; в противном случае false.
    bool Render(struct RenderPacket& packet, f32 DeltaTime) override;
    /// @brief Функция изменения размера окна игры
    /// @param Width ширина окна в пикселях.
    /// @param Height высота окна в пикселях.
    void OnResize(u32 width, u32 height) override;

    /// @brief Завершает игру, вызывая высвобождение ресурсов.
    void Shutdown() override;

    void SetupCommands();
    void SetupKeymaps();

private:
    bool ConfigureRenderViews();
    static bool OnEvent(u16 code, void *sender, void *ListenerInst, EventContext context);
    static bool OnDebugEvent(u16 code, void *sender, void *ListenerInst, EventContext context);
};

=======
#include "debug_console.hpp"

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
>>>>>>> Stashed changes
