#pragma once

#include <game_types.hpp>
#include <math/matrix4d.hpp>

class Game : public GameTypes
{
private:
    struct GameState{
        f32 DeltaTime;
        Matrix4D view;
        Vector3D<f32> CameraPosition;
        Vector3D<f32> CameraEuler;
        bool CameraViewDirty;
    }* gameState;
    
public:
    Game(i16 StartPosX, i16 StartPosY, i16 StartWidth, i16 StartHeight, const char* name);
    ~Game();

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
    bool Render(f32 DeltaTime) override;
    /// @brief Функция изменения размера окна игры
    /// @param Width ширина окна в пикселях.
    /// @param Height высота окна в пикселях.
    void OnResize(u32 Width, u32 Height) override;

    //void* operator new(u64 size);
    //void operator delete(void* ptr);

private:
    void RecalculateViewMatrix();
    void CameraYaw(f32 amount);
    void CameraPitch(f32 amount);
};

