#pragma once

#include "core/application.hpp"
#include "core/logger.hpp"
#include "core/mmemory.hpp"
#include "game_types.hpp"

#include <new>

// Внешне определенная функция для создания игры.
extern bool CreateGame(Game* OutGame);

// Основная точка входа в приложение.Основная точка входа в приложение.
int main(void) {
    
    //mem.Allocate(sizeof(Game), MEMORY_TAG_GAME);
    // Запросите экземпляр игры из приложения.
    Game* GameInst;
    
    if (!CreateGame(GameInst)) {
        MFATAL("Не удалось создать игру!");
        return -1;
    }

    /*// Ensure the function pointers exist.
    if (!GameInst.Render || !GameInst.Update || !GameInst.Initialize || !GameInst.on_resize) {
        MFATAL("The game's function pointers must be assigned!");
        return -2;
    }*/

    // Инициализация.
    if (!ApplicationCreate(GameInst)) {
        MINFO("Приложение не удалось создать!");
        return 1;
    }

    // Начать игровой цикл.
    if(!ApplicationRun()) {
        MINFO("Приложение не завершило работу корректно.");
        return 2;
    }

    // GameInst->mem->ShutDown();

    return 0;
}