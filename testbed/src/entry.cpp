#include <entry.hpp>

bool CreateGame(Game* OutGame)
{
    OutGame = new Game();
    OutGame->AppConfig.name = "Moon Engine";
    OutGame->AppConfig.StartPosX = 100;
    OutGame->AppConfig.StartPosY = 100;
    OutGame->AppConfig.StartHeight = 1280;
    OutGame->AppConfig.StartWidth = 720;

    return true;
}