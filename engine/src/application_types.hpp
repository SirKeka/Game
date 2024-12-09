/// @file application_types.hpp
/// @author 
/// @brief Этот файл содержит типы, которые будут использоваться игровой библиотекой.
/// @version 1.0
/// @date 
/// 
/// @copyright 
#pragma once

#include "core/engine.hpp"
#include "core/mmemory.hpp"

template class DArray<MString>;

struct ApplicationFrameData {
    // Динамический массив мировой геометрии, которая отрисовывается в этом кадре.
    DArray<GeometryRenderData> WorldGeometries;
};

// Представляет базовое состояние игры. Вызывается для создания приложением.
class MAPI Application
{
public:
    ApplicationConfig AppConfig;
    Engine* engine;                             // Указатель на блок памяти в котором хранится движок. Создается и управляется движком.
    void* state;                                // Указатель на блок памяти в котором хранится состояние игры. Создается и управляется игрой.
    u64 StateMemoryRequirement;                 // Требуемый размер для игрового состояния.
    LinearAllocator FrameAllocator;             // Распределитель, используемый для распределений, которые необходимо делать в каждом кадре. Содержимое стирается в начале кадра.
    DArray<GeometryRenderData> WorldGeometries; // ApplicationFrameData FrameData;        // Данные, которые создаются, используются и удаляются в каждом кадре
    // RendererPlugin* RenderPlugin;               // ЗАДАЧА: Переместить это в лучшее место...

    constexpr Application(const ApplicationConfig& AppConfig, u64 StateMemoryRequirement) : AppConfig(AppConfig), engine(nullptr), state(nullptr), StateMemoryRequirement(StateMemoryRequirement), FrameAllocator(), WorldGeometries() {}
    virtual ~Application() = default;

    /// @brief Указатель функции на последовательность загрузки игры. Это должно заполнить конфигурацию приложения конкретными требованиями игры.
    /// @return True в случае успеха; в противном случае false.
    virtual bool Boot() = 0;

    /// @brief Функция инициализации
    /// @return true в случае успеха; в противном случае false.
    virtual bool Initialize() = 0;

    /// @brief Функция обновления игры
    /// @param DeltaTime Время в секундах с момента последнего кадра.
    /// @return true в случае успеха; в противном случае false.
    virtual bool Update(f32 DeltaTime) = 0;

    /// @brief Функция рендеринга игры
    /// @param packet A pointer to the packet to be populated by the game.
    /// @param DeltaTime Время в секундах с момента последнего кадра.
    /// @return true в случае успеха; в противном случае false.
    virtual bool Render(struct RenderPacket& packet, f32 DeltaTime) = 0;

    /// @brief Функция изменения размера окна игры
    /// @param Width ширина окна в пикселях.
    /// @param Height высота окна в пикселях.
    virtual void OnResize(u32 Width, u32 Height) = 0;

    /// @brief Завершает игру, вызывая высвобождение ресурсов.
    virtual void Shutdown() = 0;
};
