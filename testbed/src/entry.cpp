#include <entry.hpp>

bool CreateGame(Game*& OutGame)
{
    OutGame = new Game(100, 100, 720, 1280, "Moon Engine");

    return true;
}