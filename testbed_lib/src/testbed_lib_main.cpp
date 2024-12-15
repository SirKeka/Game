#include "testbed_lib_main.hpp"
#include "game.hpp"

<<<<<<< Updated upstream
Application *CreateGame(const ApplicationConfig& config)
{
    return new Game(config);
=======
bool CreateGame(const ApplicationConfig& config)
{
    return true;
>>>>>>> Stashed changes
}

void DestroyGame(Application *application)
{
}
