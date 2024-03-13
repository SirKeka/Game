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
    
    CameraPosition = Vector3D<f32> (0, 0, 30.f);
    CameraEuler = Vector3D<f32>::Zero();

    view = Matrix4::MakeTranslation(CameraPosition);
    view.Inverse();
    CameraViewDirty = true;
    
    return true;
}

bool Game::Update(f32 DeltaTime)
{
    static u64 AllocCount = 0;
    u64 PrevAllocCount = AllocCount;
    AllocCount = MMemory::GetMemoryAllocCount(); 
    if (Input::IsKeyUp(KEY_M) && Input::WasKeyDown(KEY_M)) {
        MDEBUG("Распределено: %llu (%llu в этом кадре)", AllocCount, AllocCount - PrevAllocCount);
    }

    // ВЗЛОМ: временный взлом для перемещения камеры по кругу.
    if (Input::IsKeyDown(KEY_A) || Input::IsKeyDown(KEY_LEFT)) {
        CameraYaw(1.0f * DeltaTime);
    }

    if (Input::IsKeyDown(KEY_D) || Input::IsKeyDown(KEY_RIGHT)) {
        CameraYaw(-1.0f * DeltaTime);
    }

    if (Input::IsKeyDown(KEY_UP)) {
        CameraPitch(1.0f * DeltaTime);
    }

    if (Input::IsKeyDown(KEY_DOWN)) {
        CameraPitch(-1.0f * DeltaTime);
    }

    f32 TempMoveSpeed = 50.0f;
    Vector3D<f32> velocity = Vector3D<f32>::Zero();

    //Не работает
    if (Input::IsKeyDown(KEY_W)) {
        Vector3D<f32> forward = Matrix4::Forward(view);
        velocity += forward;
    }

    //Не работает
    if (Input::IsKeyDown(KEY_S)) {
        Vector3D<f32> backward = Matrix4::Backward(view);
        velocity += backward;
    }

    if (Input::IsKeyDown(KEY_Q)) {
        Vector3D<f32> left = Matrix4::Left(view);
        velocity += left;
    }

    if (Input::IsKeyDown(KEY_E)) {
        Vector3D<f32> right = Matrix4::Right(view);
        velocity += right;
    }

    if (Input::IsKeyDown(KEY_SPACE)) {
        velocity.y -= 1.0f;
    }

    if (Input::IsKeyDown(KEY_X)) {
        velocity.y += 1.0f;
    }

    Vector3D<f32> z = Vector3D<f32>::Zero();
    if (!Compare(z, velocity, 0.0002f)) {
        // Обязательно нормализуйте скорость перед применением.
        velocity.Normalize();
        CameraPosition.x += velocity.x * TempMoveSpeed * DeltaTime;
        CameraPosition.y += velocity.y * TempMoveSpeed * DeltaTime;
        CameraPosition.z += velocity.z * TempMoveSpeed * DeltaTime;
        CameraViewDirty = true;
    }

    RecalculateViewMatrix();

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
