#include "game.hpp"

#include <core/logger.hpp>
#include <renderer/renderer.hpp>
#include <new>

Game::Game(i16 StartPosX, i16 StartPosY, i16 StartWidth, i16 StartHeight, const char* name) 
{
    AppConfig.StartPosX = StartPosX;
    AppConfig.StartPosY = StartPosY;
    AppConfig.StartWidth = StartWidth;
    AppConfig.StartHeight = StartHeight;
    AppConfig.name = name;

    StateMemoryRequirement = sizeof(GameState);
    state = nullptr;
    application = nullptr;
}

bool Game::Initialize()
{
    MDEBUG("Game::Initialize вызван!");

    gameState = reinterpret_cast<GameState*>(state);
    
    gameState->CameraPosition = Vector3D<f32> (0, 0, 30.f);
    gameState->CameraEuler = Vector3D<f32>::Zero();

    gameState->view = Matrix4D::MakeTranslation(gameState->CameraPosition);
    gameState->view.Inverse();
    gameState->CameraViewDirty = true;
    
    return true;
}

bool Game::Update(f32 DeltaTime)
{
    u64 AllocCount = 0;
    u64 PrevAllocCount = AllocCount;
    AllocCount = MMemory::GetMemoryAllocCount(); // TODO: проверить странные значения
    if (Input::Instance()->Instance()->IsKeyUp(KEY_M) && Input::Instance()->WasKeyDown(KEY_M)) {
        MDEBUG("Распределено: %llu (%llu в этом кадре)", AllocCount, AllocCount - PrevAllocCount);
    }

    // TODO: temp
    if (Input::Instance()->IsKeyUp(KEY_T) && Input::Instance()->WasKeyDown(KEY_T)) {
        MDEBUG("Swapping texture!");
        EventContext context = {};
        Event::GetInstance()->Fire(EVENT_CODE_DEBUG0, this, context);
    }
    // TODO: end temp

    // ВЗЛОМ: временный взлом для перемещения камеры по кругу.
    if (Input::Instance()->IsKeyDown(KEY_A) || Input::Instance()->IsKeyDown(KEY_LEFT)) {
        CameraYaw(1.0f * DeltaTime);
    }

    if (Input::Instance()->IsKeyDown(KEY_D) || Input::Instance()->IsKeyDown(KEY_RIGHT)) {
        CameraYaw(-1.0f * DeltaTime);
    }

    if (Input::Instance()->IsKeyDown(KEY_UP)) {
        CameraPitch(1.0f * DeltaTime);
    }

    if (Input::Instance()->IsKeyDown(KEY_DOWN)) {
        CameraPitch(-1.0f * DeltaTime);
    }

    f32 TempMoveSpeed = 50.0f;
    Vector3D<f32> velocity = Vector3D<f32>::Zero();

    //Не работает
    if (Input::Instance()->IsKeyDown(KEY_W)) {
        Vector3D<f32> forward = Matrix4::Forward(gameState->view);
        velocity += forward;
    }

    //Не работает
    if (Input::Instance()->IsKeyDown(KEY_S)) {
        Vector3D<f32> backward = Matrix4::Backward(gameState->view);
        velocity += backward;
    }

    if (Input::Instance()->IsKeyDown(KEY_Q)) {
        Vector3D<f32> left = Matrix4::Left(gameState->view);
        velocity += left;
    }

    if (Input::Instance()->IsKeyDown(KEY_E)) {
        Vector3D<f32> right = Matrix4::Right(gameState->view);
        velocity += right;
    }

    if (Input::Instance()->IsKeyDown(KEY_SPACE)) {
        velocity.y += 1.0f;
    }

    if (Input::Instance()->IsKeyDown(KEY_X)) {
        velocity.y -= 1.0f;
    }

    Vector3D<f32> z = Vector3D<f32>::Zero();
    if (!Compare(z, velocity, 0.0002f)) {
        // Обязательно нормализуйте скорость перед применением.
        velocity.Normalize();
        gameState->CameraPosition.x += velocity.x * TempMoveSpeed * DeltaTime;
        gameState->CameraPosition.y += velocity.y * TempMoveSpeed * DeltaTime;
        gameState->CameraPosition.z += velocity.z * TempMoveSpeed * DeltaTime;
        gameState->CameraViewDirty = true;
    }

    RecalculateViewMatrix();

    application->State->Render->SetView(gameState->view);

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

/*void *Game::operator new(u64 size)
{   
    return MMemory::Allocate(size, MemoryTag::Game);
}

void Game::operator delete(void *ptr)
{
    MMemory::Free(ptr,sizeof(Game), MemoryTag::Game);
}*/

void Game::RecalculateViewMatrix()
{
    if(gameState->CameraViewDirty) {
        Matrix4D rotation = Matrix4::MakeEulerXYZ(gameState->CameraEuler);
        Matrix4D translation = Matrix4D::MakeTranslation(gameState->CameraPosition);

        gameState->view = rotation * translation;
        gameState->view.Inverse();

        gameState->CameraViewDirty = false;
    }
}

void Game::CameraYaw(f32 amount)
{
    gameState->CameraEuler.y += amount;
    gameState->CameraViewDirty = true;
}

void Game::CameraPitch(f32 amount)
{
    gameState->CameraEuler.x += amount;

    // Зажмите, чтобы избежать блокировки Gimball.
    f32 limit = Math::DegToRad(89.0f);
    gameState->CameraEuler.x = MCLAMP(gameState->CameraEuler.x, -limit, limit);

    gameState->CameraViewDirty = true;
}
