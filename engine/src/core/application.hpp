#pragma once

#include "defines.hpp"

class Game;

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

class Application
{
public:
    // MMemory* mem;
    Input* Inputs;
    Event* Events;
    bool IsRunning;
    bool IsSuspended;
    MWindow* Window;
    Game* GameInst;
    
    i16 width;
    i16 height;
    f64 LastTime;
public:
    Application(/* args */);
    ~Application();
};

MAPI bool ApplicationCreate(Game* GameInst);

MAPI bool ApplicationRun();