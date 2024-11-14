#include "game.hpp"

#include <core/console.hpp>

#include <core/event.hpp>

void GameCommandExit(ConsoleCommandContext context) {
    MDEBUG("Game exit called!");
    EventSystem::Fire(EVENT_CODE_APPLICATION_QUIT, nullptr, (EventContext){});
}

void Game::SetupCommands() {
    Console::RegisterCommand("exit", 0, GameCommandExit);
    Console::RegisterCommand("quit", 0, GameCommandExit);
}