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
}

bool Game::Initialize()
{
    MDEBUG("game_initialize() called!");
    return true;
}

bool Game::Update(f32 DeltaTime)
{
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
    MDEBUG("Выделено: %d", size, "байт");
    
    return MMemory::Allocate(size, MEMORY_TAG_GAME);
}

void Game::operator delete(void *ptr)
{
    MMemory::Free(ptr,sizeof(Game), MEMORY_TAG_GAME);
}
