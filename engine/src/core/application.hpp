#pragma once

#include "defines.hpp"

struct Game;

// Конфигурация приложения.
struct ApplicationConfig {
    // Начальное положение окна по оси X, если применимо.
    i16 StartPosX;

    // Начальное положение окна по оси Y, если применимо.
    i16 StartPosY;

    // Начальная ширина окна, если применимо.
    i16 StartWidth;

    // Начальная высота окна, если применимо.
    i16 StartHeight;

    // Имя приложения, используемое в оконном режиме, если применимо.
    const char* name;
};

MAPI bool ApplicationCreate(ApplicationConfig* Config);

MAPI bool ApplicationRun();