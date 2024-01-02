#pragma once

#include "core/application.hpp"
#include "core/memory.hpp"

/* Представляет базовое состояние игры.
 * Вызывается для создания приложением. */
class Game
{
private:
    
public:
    // Конфигурация приложения.
    ApplicationConfig AppConfig;
    Memory mem;
    MAPI Game();
    // Функция инициализации
    bool Initialize(/*Game* GameInst*/);                                                                                      
    // Функция обновления игры
    bool Update(/*Game* GameInst,*/ f32 DeltaTime);
    // Функция рендеринга игры
    bool Render(/*Game* GameInst,*/ f32 DeltaTime);
    // Функция изменения размера окна игры
    void OnResize(/*Game* GameInst,*/ u32 Width, u32 Height);
    ~Game();
};
