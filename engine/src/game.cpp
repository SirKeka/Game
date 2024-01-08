#include "game_types.hpp"

#include <core/logger.hpp>
#include <new>

MMemory Game::mem = MMemory();

/*Game::Game() : mem(mem)
{

}*/

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
    MINFO("Выделено: %d", size, "байт");
    
    return mem.Allocate(size, MEMORY_TAG_GAME);
}

void Game::operator delete(void *ptr)
{
    mem.Free(ptr,sizeof(Game), MEMORY_TAG_GAME);
}
