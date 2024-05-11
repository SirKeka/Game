/// @file game_types.hpp
/// @author 
/// @brief Этот файл содержит типы, которые будут использоваться игровой библиотекой.
/// @version 1.0
/// @date 
/// 
/// @copyright 
#pragma once

#include "core/application.hpp"
#include "core/mmemory.hpp"

// Конфигурация приложения.
struct ApplicationConfig {
    i16 StartPosX;      // Начальное положение окна по оси X, если применимо.
    i16 StartPosY;      // Начальное положение окна по оси Y, если применимо.
    i16 StartWidth;     // Начальная ширина окна, если применимо.
    i16 StartHeight;    // Начальная высота окна, если применимо.
    const char* name;   // Имя приложения, используемое в оконном режиме, если применимо.
    
};

// Представляет базовое состояние игры. Вызывается для создания приложением.
class GameTypes
{
private:

public:
    ApplicationConfig AppConfig;
    Application* application;   // Указатель на блок памяти в котором храненится состояние приложения. Создается и управляется движком.
    void* state;                // Указатель на блок памяти в котором храненится состояние игры. Создается и управляется игрой.
    u64 StateMemoryRequirement; // Требуемый размер для игрового состояния.
    
    //GameTypes();
    virtual ~GameTypes() = default;
    /// @brief Функция инициализации
    /// @return true в случае успеха; в противном случае false.
    virtual bool Initialize() = 0;
    /// @brief Функция обновления игры
    /// @param DeltaTime Время в секундах с момента последнего кадра.
    /// @return true в случае успеха; в противном случае false.
    virtual bool Update(f32 DeltaTime) = 0;
    /// @brief Функция рендеринга игры
    /// @param DeltaTime Время в секундах с момента последнего кадра.
    /// @return true в случае успеха; в противном случае false.
    virtual bool Render(f32 DeltaTime) = 0;
    /// @brief Функция изменения размера окна игры
    /// @param Width ширина окна в пикселях.
    /// @param Height высота окна в пикселях.
    virtual void OnResize(u32 Width, u32 Height) = 0;
};
