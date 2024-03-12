#pragma once

#include "core/application.hpp"
#include "core/logger.hpp"
#include "core/mmemory.hpp"
#include "game_types.hpp"

// Внешне определенная функция для создания игры.
extern bool CreateGame(GameTypes*& OutGame);

// Основная точка входа в приложение.Основная точка входа в приложение.
int main(void) {
    system("chcp 65001 > nul"); // для отображения русских символов в консоли

    // Запросите экземпляр игры из приложения.
    GameTypes* GameInst;
    
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
    if (!GameInst->State->ApplicationCreate(GameInst)) {
        MINFO("Приложение не удалось создать!");
        return 1;
    }

    // Начать игровой цикл.
    if(!GameInst->State->ApplicationRun()) {
        MINFO("Приложение не завершило работу корректно.");
        return 2;
    }

    return 0;
}