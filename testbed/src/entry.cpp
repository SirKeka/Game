#include <entry.hpp>
#include "game.hpp"

bool CreateApplication(Application*& OutApplication)
{
    ApplicationConfig config;
    config.StartPosX = 100;
    config.StartPosY = 100;
    config.StartWidth = 1280;
    config.StartHeight = 720;
    config.name = "Moon Engine";
    // config.FontConfig = 
    // config.RenderViews = 
    OutApplication = new Game(config);

    return true;
}