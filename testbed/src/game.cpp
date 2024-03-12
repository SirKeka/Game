#include "game.hpp"

#include <core/logger.hpp>
#include <new>

Game::Game(i16 StartPosX, i16 StartPosY, i16 StartWidth, i16 StartHeight, const char* name) 
{
    AppConfig.StartPosX = StartPosX;
    AppConfig.StartPosY = StartPosY;
    AppConfig.StartWidth = StartWidth;
    AppConfig.StartHeight = StartHeight;
    AppConfig.name = name;

    State = nullptr;
}

bool Game::Initialize()
{
    MDEBUG("game_initialize() called!");
    return true;

    CameraPosition = Vector3D<f32> (0, 0, -30.f);
    CameraEuler = Vector3D<f32>::Zero();

    view = Matrix4::MakeTranslation(CameraPosition);
    view.Inverse();
}

bool Game::Update(f32 DeltaTime)
{
    static u64 AllocCount = 0;
    u64 PrevAllocCount = AllocCount;
    AllocCount = MMemory::GetMemoryAllocCount(); 
    if (Input::IsKeyUp(KEY_M) && Input::WasKeyDown(KEY_M)) {
        MDEBUG("Распределено: %llu (%llu в этом кадре)", AllocCount, AllocCount - PrevAllocCount);
    }
    State->AppState->Render->SetView(view);

    return true;
}

bool Game::Render(f32 DeltaTime) 
{
    return true;
}

void Game::OnResize(u32 Width, u32 Height) 
{

}

Game::~Game()
{
}

void *Game::operator new(u64 size)
{   
    return MMemory::Allocate(size, MEMORY_TAG_GAME);
}

void Game::operator delete(void *ptr)
{
    MMemory::Free(ptr,sizeof(Game), MEMORY_TAG_GAME);
}

void Game::RecalculateViewMatrix()
{
    if(CameraViewDirty) {
        Matrix4D rotation = Matrix4::MakeEulerXYZ(CameraEuler);
        Matrix4D translation = Matrix4::MakeTranslation(CameraPosition);

        view = rotation * translation;
        view.Inverse();

        CameraViewDirty = false;
    }
}

void Game::CameraYaw(f32 amount)
{
    CameraEuler.y += amount;
    CameraViewDirty = true;
}

void Game::CameraPitch(f32 amount)
{
    CameraEuler.x += amount;

    // Зажмите, чтобы избежать блокировки Gimball.
    f32 limit = Math::DegToRad(89.0f);
    CameraEuler.x = MCLAMP(CameraEuler.x, -limit, limit);

    CameraViewDirty = true;
}
