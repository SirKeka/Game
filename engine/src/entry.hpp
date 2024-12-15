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
#include "application_types.hpp"

#include <stdlib.h>
#include <locale.h>

/// @brief Определенная извне функция для создания приложения, предоставленная потребителем этой библиотеки.
/// @return True при успешном создании; в противном случае false.
extern bool CreateApplication(Application& OutApplication);

extern bool InitializeApplication(Application& app);

// Основная точка входа в приложение.
int main(void) {
    system("chcp 65001 > nul"); // для отображения русских символов в консоли
    // setlocale(LC_ALL, "en_US.UTF-8");

    // Запросите экземпляр игры из приложения.
    Application ApplicationInst;

    // Система памяти должна быть установлена в первую очередь. 
    if (!MemorySystem::Initialize(GIBIBYTES(1))) {
        MERROR("Не удалось инициализировать систему памяти!");
        return -1;
    }
    
    if (!CreateApplication(ApplicationInst)) {
        MFATAL("Не удалось создать конфигурацию игры!");
        return -2;
    }
    
    // Инициализация.
    if (!Engine::Create(ApplicationInst)) {
        MINFO("Не удалось создать движок!");
        return 1;
    }

    if (!InitializeApplication(ApplicationInst)) {
        MFATAL("Не удалось инициализировать приложение!");
        return -1;
    }

    // Начать игровой цикл.
    if(!ApplicationInst.engine->Run()) {
        MINFO("Приложение не завершило работу корректно.");
        return 2;
    }

    MemorySystem::Shutdown();

    return 0;
}