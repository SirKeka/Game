#include "game.hpp"

#include <core/logger.hpp>
#include <systems/camera_system.hpp>
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
    
    gameState->WorldCamera = CameraSystem::Instance()->GetDefault();
    gameState->WorldCamera->SetPosition(FVec3(10.5F, 5.0F, 9.5f));
    
    return true;
}

bool Game::Update(f32 DeltaTime)
{
    u64 AllocCount = 0;
    u64 PrevAllocCount = AllocCount;
    AllocCount = MMemory::GetMemoryAllocCount(); // TODO: проверить странные значения
    if (Input::Instance()->Instance()->IsKeyUp(Keys::M) && Input::Instance()->WasKeyDown(Keys::M)) {
        MDEBUG("Распределено: %llu (%llu в этом кадре)", AllocCount, AllocCount - PrevAllocCount);
    }

    // TODO: temp
    if (Input::Instance()->IsKeyUp(Keys::T) && Input::Instance()->WasKeyDown(Keys::T)) {
        MDEBUG("Swapping texture!");
        EventContext context = {};
        Event::GetInstance()->Fire(EVENT_CODE_DEBUG0, this, context);
    }
    // TODO: end temp

    // ВЗЛОМ: временный взлом для перемещения камеры по кругу.
    if (Input::Instance()->IsKeyDown(Keys::A) || Input::Instance()->IsKeyDown(Keys::LEFT)) {
        gameState->WorldCamera->Yaw(1.0f * DeltaTime);
    }

    if (Input::Instance()->IsKeyDown(Keys::D) || Input::Instance()->IsKeyDown(Keys::RIGHT)) {
        gameState->WorldCamera->Yaw(-1.0f * DeltaTime);
    }

    if (Input::Instance()->IsKeyDown(Keys::UP)) {
        gameState->WorldCamera->Pitch(1.0f * DeltaTime);
    }

    if (Input::Instance()->IsKeyDown(Keys::DOWN)) {
        gameState->WorldCamera->Pitch(-1.0f * DeltaTime);
    }

    static const f32 TempMoveSpeed = 50.0f;

    if (Input::Instance()->IsKeyDown(Keys::W)) {
        gameState->WorldCamera->MoveForward(TempMoveSpeed * DeltaTime);
    }

    if (Input::Instance()->IsKeyDown(Keys::S)) {
        gameState->WorldCamera->MoveBackward(TempMoveSpeed * DeltaTime);
    }

    if (Input::Instance()->IsKeyDown(Keys::Q)) {
        gameState->WorldCamera->MoveLeft(TempMoveSpeed * DeltaTime);
    }

    if (Input::Instance()->IsKeyDown(Keys::E)) {
        gameState->WorldCamera->MoveRight(TempMoveSpeed * DeltaTime);
    }

    if (Input::Instance()->IsKeyDown(Keys::SPACE)) {
        gameState->WorldCamera->MoveUp(TempMoveSpeed * DeltaTime);
    }

    if (Input::Instance()->IsKeyDown(Keys::X)) {
        gameState->WorldCamera->MoveDown(TempMoveSpeed * DeltaTime);
    }

    // СДЕЛАТЬ: временно
    if (Input::Instance()->IsKeyUp(Keys::P) && Input::Instance()->WasKeyDown(Keys::P)) {
        MDEBUG(
            "Позиция:[%.2f, %.2f, %.2f",
            gameState->WorldCamera->GetPosition().x,
            gameState->WorldCamera->GetPosition().y,
            gameState->WorldCamera->GetPosition().z);
    }

    // RENDERER DEBUG FUNCTIONS
    if (Input::Instance()->IsKeyUp(Keys::KEY_1) && Input::Instance()->WasKeyDown(Keys::KEY_1)) {
        EventContext data = {};
        data.data.i32[0] = Renderer::Lighting;
        Event::GetInstance()->Fire(EVENT_CODE_SET_RENDER_MODE, this, data);
    }

    if (Input::Instance()->IsKeyUp(Keys::KEY_2) && Input::Instance()->WasKeyDown(Keys::KEY_2)) {
        EventContext data = {};
        data.data.i32[0] = Renderer::Normals;
        Event::GetInstance()->Fire(EVENT_CODE_SET_RENDER_MODE, this, data);
    }

    if (Input::Instance()->IsKeyUp(Keys::KEY_0) && Input::Instance()->WasKeyDown(Keys::KEY_0)) {
        EventContext data = {};
        data.data.i32[0] = Renderer::Default;
        Event::GetInstance()->Fire(EVENT_CODE_SET_RENDER_MODE, this, data);
    }
    // СДЕЛАТЬ: временно

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
