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
class GameTypes
{
private:

public:
    ApplicationConfig AppConfig;
    Application* State;
    
    //GameTypes();
    virtual ~GameTypes() = default;
    // Функция инициализации
    virtual bool Initialize() = 0;                                                                                      
    // Функция обновления игры
    virtual bool Update(f32 DeltaTime) = 0;
    // Функция рендеринга игры
    virtual bool Render(f32 DeltaTime) = 0;
    // Функция изменения размера окна игры
    virtual void OnResize( u32 Width, u32 Height) = 0;
};
