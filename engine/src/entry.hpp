#pragma once

#include "core/application.hpp"
#include "core/logger.hpp"
#include "core/mmemory.hpp"
#include "game_types.hpp"

#include <stdlib.h>

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
    
    // Инициализация.
    if (!GameInst->application->Create(GameInst)) {
        MINFO("Приложение не удалось создать!");
        return 1;
    }

    // Начать игровой цикл.
    if(!GameInst->application->ApplicationRun()) {
        MINFO("Приложение не завершило работу корректно.");
        return 2;
    }

    return 0;
}