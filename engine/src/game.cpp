#include "game_types.hpp"

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
}

bool Game::Update(f32 DeltaTime)
{
    static u64 AllocCount = 0;
    u64 PrevAllocCount = AllocCount;
    AllocCount = MMemory::GetMemoryAllocCount(); 
    if (Input::IsKeyUp(KEY_M) && Input::WasKeyDown(KEY_M)) {
        MDEBUG("Распределено: %llu (%llu в этом кадре)", AllocCount, AllocCount - PrevAllocCount);
    }
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
