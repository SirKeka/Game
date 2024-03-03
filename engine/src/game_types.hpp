#pragma once

#include "core/application.hpp"
#include "core/mmemory.hpp"

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

/* Представляет базовое состояние игры.
 * Вызывается для создания приложением. */
class Game
{
private:

public:
    ApplicationConfig AppConfig;
    Application* State;
    
    MAPI Game(i16 StartPosX, i16 StartPosY, i16 StartWidth, i16 StartHeight, const char* name);
    // Функция инициализации
    bool Initialize();                                                                                      
    // Функция обновления игры
    bool Update(f32 DeltaTime);
    // Функция рендеринга игры
    bool Render(f32 DeltaTime);
    // Функция изменения размера окна игры
    void OnResize( u32 Width, u32 Height);
    ~Game();
    MAPI void* operator new(u64 size);
    MAPI void operator delete(void* ptr);
};
