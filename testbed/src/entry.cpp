#include <entry.hpp>
#include "game.hpp"

bool CreateGame(GameTypes& OutGame)
{
    OutGame = new Game(100, 100, 720, 1280, "Moon Engine");

    return true;
}