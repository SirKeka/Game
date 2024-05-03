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
    }* state;
    
public:
    Game(i16 StartPosX, i16 StartPosY, i16 StartWidth, i16 StartHeight, const char* name);
    ~Game();

    // Функция инициализации
    bool Initialize() override;                                                                                      
    // Функция обновления игры
    bool Update(f32 DeltaTime) override;
    // Функция рендеринга игры
    bool Render(f32 DeltaTime) override;
    // Функция изменения размера окна игры
    void OnResize( u32 Width, u32 Height) override;

    void* operator new(u64 size);
    void operator delete(void* ptr);

private:
    void RecalculateViewMatrix();
    void CameraYaw(f32 amount);
    void CameraPitch(f32 amount);
};

