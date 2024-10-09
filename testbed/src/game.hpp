#pragma once

#include <game_types.hpp>

class Game : public GameTypes
{
public:
    struct State{
        f32 DeltaTime;
        class Camera* WorldCamera;

        u16 width, height;

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
        // ЗАДАЧА: конец временно
    };
    
public:
    constexpr Game(const ApplicationConfig& config) : GameTypes(config, sizeof(State)) {}
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

private:
    bool ConfigureRenderViews();
    static bool OnEvent(u16 code, void *sender, void *ListenerInst, EventContext context);
    static bool OnDebugEvent(u16 code, void *sender, void *ListenerInst, EventContext context);
};

