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
    auto InputInst = Input::Instance();
    auto EventIns  = Event::GetInstance();
    static u64 AllocCount = 0;
    u64 PrevAllocCount = AllocCount;
    AllocCount = MMemory::GetMemoryAllocCount(); // TODO: проверить странные значения
<<<<<<< Updated upstream
    if (InputInst->IsKeyUp(Keys::M) && InputInst->WasKeyDown(Keys::M)) {        
=======
    if (InputInst->IsKeyUp(Keys::M) && InputInst->WasKeyDown(Keys::M)) {
        
>>>>>>> Stashed changes
        MINFO(MMemory::GetMemoryUsageStr().c_str());
        MDEBUG("Распределено: %llu (%llu в этом кадре)", AllocCount, AllocCount - PrevAllocCount);
    }

    // TODO: temp
    if (InputInst->IsKeyUp(Keys::T) && InputInst->WasKeyDown(Keys::T)) {
        MDEBUG("Swapping texture!");
        EventContext context = {};
        EventIns->Fire(EVENT_CODE_DEBUG0, this, context);
    }
    // TODO: end temp

    // ВЗЛОМ: временный взлом для перемещения камеры по кругу.
    if (InputInst->IsKeyDown(Keys::A) || InputInst->IsKeyDown(Keys::LEFT)) {
        gameState->WorldCamera->Yaw(1.0f * DeltaTime);
    }

    if (InputInst->IsKeyDown(Keys::D) || InputInst->IsKeyDown(Keys::RIGHT)) {
        gameState->WorldCamera->Yaw(-1.0f * DeltaTime);
    }

    if (InputInst->IsKeyDown(Keys::UP)) {
        gameState->WorldCamera->Pitch(1.0f * DeltaTime);
    }

    if (InputInst->IsKeyDown(Keys::DOWN)) {
        gameState->WorldCamera->Pitch(-1.0f * DeltaTime);
    }

    static const f32 TempMoveSpeed = 50.0f;

    if (InputInst->IsKeyDown(Keys::W)) {
        gameState->WorldCamera->MoveForward(TempMoveSpeed * DeltaTime);
    }

    if (InputInst->IsKeyDown(Keys::S)) {
        gameState->WorldCamera->MoveBackward(TempMoveSpeed * DeltaTime);
    }

    if (InputInst->IsKeyDown(Keys::Q)) {
        gameState->WorldCamera->MoveLeft(TempMoveSpeed * DeltaTime);
    }

    if (InputInst->IsKeyDown(Keys::E)) {
        gameState->WorldCamera->MoveRight(TempMoveSpeed * DeltaTime);
    }

    if (InputInst->IsKeyDown(Keys::SPACE)) {
        gameState->WorldCamera->MoveUp(TempMoveSpeed * DeltaTime);
    }

    if (InputInst->IsKeyDown(Keys::X)) {
        gameState->WorldCamera->MoveDown(TempMoveSpeed * DeltaTime);
    }

    // СДЕЛАТЬ: временно
    if (InputInst->IsKeyUp(Keys::P) && InputInst->WasKeyDown(Keys::P)) {
        MDEBUG(
            "Позиция:[%.2f, %.2f, %.2f",
            gameState->WorldCamera->GetPosition().x,
            gameState->WorldCamera->GetPosition().y,
            gameState->WorldCamera->GetPosition().z);
    }

    // RENDERER DEBUG FUNCTIONS
    if (InputInst->IsKeyUp(Keys::KEY_1) && InputInst->WasKeyDown(Keys::KEY_1)) {
        EventContext data = {};
        data.data.i32[0] = Renderer::Lighting;
        EventIns->Fire(EVENT_CODE_SET_RENDER_MODE, this, data);
    }

    if (InputInst->IsKeyUp(Keys::KEY_2) && InputInst->WasKeyDown(Keys::KEY_2)) {
        EventContext data = {};
        data.data.i32[0] = Renderer::Normals;
        EventIns->Fire(EVENT_CODE_SET_RENDER_MODE, this, data);
    }

    if (InputInst->IsKeyUp(Keys::KEY_0) && InputInst->WasKeyDown(Keys::KEY_0)) {
        EventContext data = {};
        data.data.i32[0] = Renderer::Default;
        EventIns->Fire(EVENT_CODE_SET_RENDER_MODE, this, data);
    }

    // ЗАДАЧА: временно
    // Привяжите клавишу для загрузки некоторых данных.
    if (InputInst->IsKeyUp(Keys::L) && InputInst->WasKeyDown(Keys::L)) {
        EventContext context = {};
        EventIns->Fire(EVENT_CODE_DEBUG1, this, context);
    }

    // ЗАДАЧА: временно

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
