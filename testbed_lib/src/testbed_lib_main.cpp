#include "testbed_lib_main.hpp"
#include "game.hpp"

Application *CreateGame(const ApplicationConfig& config)
{
    return new Game(config);
}

void DestroyGame(Application *application)
{
}
