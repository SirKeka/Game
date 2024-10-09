#include <entry.hpp>
#include "game.hpp"

bool CreateGame(GameTypes*& OutGame)
{
    ApplicationConfig config;
    config.StartPosX = 100;
    config.StartPosY = 100;
    config.StartWidth = 720;
    config.StartHeight = 1280;
    config.name = "Moon Engine";
    // config.FontConfig = 
    // config.RenderViews = 
    OutGame = new Game(config);

    return true;
}