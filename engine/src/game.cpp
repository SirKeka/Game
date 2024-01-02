#include "game_types.hpp"

#include <core/logger.hpp>

Game::Game()
{
    mem = Memory();
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

