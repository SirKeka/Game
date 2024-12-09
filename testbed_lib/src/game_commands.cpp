#include "game.hpp"

#include <core/console.hpp>

#include <core/event.hpp>

void GameCommandExit(ConsoleCommandContext context) {
    MDEBUG("Команда выход из игры вызвана!");
    EventSystem::Fire(EventSystem::ApplicationQuit, nullptr, (EventContext){});
}

void Game::SetupCommands() {
    Console::RegisterCommand("exit", 0, GameCommandExit);
    Console::RegisterCommand("quit", 0, GameCommandExit);
}