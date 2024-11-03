/// @file entry.hpp
/// @author 
/// @brief Этот файл содержит основную точку входа в приложение.
/// Он также содержит ссылку на внешне определенный метод CreateApplication(), 
/// который должен создать и установить пользовательский объект приложения в местоположении, 
/// указанном OutApplication. Это будет предоставлено потребляющим приложением, 
/// которое затем подключается к самому движку во время фазы загрузки.
/// @version 1.0
/// @date 
/// 
/// @copyright Kohi Game Engine is Copyright (c) Travis Vroman 2021-2022

#pragma once

#include "core/engine.hpp"
#include "core/logger.hpp"
#include "core/mmemory.hpp"
#include "application_types.hpp"

#include <stdlib.h>

// Внешне определенная функция для создания игры.

/// @brief Определенная извне функция для создания приложения, предоставленная потребителем этой библиотеки.
/// @param OutApplication Ссылка на созданный объект приложения, предоставленный потребителем.
/// @return True при успешном создании; в противном случае false.
extern bool CreateApplication(Application*& OutApplication);

// Основная точка входа в приложение.Основная точка входа в приложение.
int main(void) {
    system("chcp 65001 > nul"); // для отображения русских символов в консоли


    // Запросите экземпляр игры из приложения.
    Application* ApplicationInst;
    
    if (!CreateApplication(ApplicationInst)) {
        MFATAL("Не удалось создать игру!");
        return -1;
    }
    
    // Инициализация.
    if (!ApplicationInst->engine->Create(ApplicationInst)) {
        MINFO("Приложение не удалось создать!");
        return 1;
    }

    // Начать игровой цикл.
    if(!ApplicationInst->engine->Run()) {
        MINFO("Приложение не завершило работу корректно.");
        return 2;
    }

    return 0;
}