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

struct RenderPacket;

// Представляет базовое состояние игры. Вызывается для создания приложением.
struct Application
{
    ApplicationConfig AppConfig;
    Engine* engine;                             // Указатель на блок памяти в котором хранится движок. Создается и управляется движком.
    void* state;                                // Указатель на блок памяти в котором хранится состояние игры. Создается и управляется игрой.
    u64 StateMemoryRequirement;                 // Требуемый размер для игрового состояния.
    LinearAllocator FrameAllocator;             // Распределитель, используемый для распределений, которые необходимо делать в каждом кадре. Содержимое стирается в начале кадра.
    DArray<GeometryRenderData> WorldGeometries; // ApplicationFrameData FrameData;        // Данные, которые создаются, используются и удаляются в каждом кадре
    
    // ЗАДАЧА: Переместить это в лучшее место...
    DynamicLibrary RendererLibrary;
    DynamicLibrary GameLibrary;

    /// @brief Указатель функции на последовательность загрузки игры. Это должно заполнить конфигурацию приложения конкретными требованиями игры.
    /// @return True в случае успеха; в противном случае false.
    bool (*Boot)(Application& app) = nullptr;

    /// @brief Функция инициализации
    /// @return true в случае успеха; в противном случае false.
    bool (*Initialize)(Application& app) = nullptr;

    /// @brief Функция обновления игры
    /// @param DeltaTime Время в секундах с момента последнего кадра.
    /// @return true в случае успеха; в противном случае false.
    bool (*Update)(Application& app, f32 DeltaTime) = nullptr;

    /// @brief Функция рендеринга игры
    /// @param packet ссылка на пакет, который будет заполнен игрой.
    /// @param DeltaTime Время в секундах с момента последнего кадра.
    /// @return true в случае успеха; в противном случае false.
    bool (*Render)(Application& app, RenderPacket& packet, f32 DeltaTime) = nullptr;

    /// @brief Функция изменения размера окна игры
    /// @param Width ширина окна в пикселях.
    /// @param Height высота окна в пикселях.
    void (*OnResize)(Application& app, u32 Width, u32 Height) = nullptr;

    /// @brief Завершает игру, вызывая высвобождение ресурсов.
    void (*Shutdown)(Application& app) = nullptr;

    void (*LibOnLoad)(Application& app) = nullptr;

    void (*LibOnUnload)(Application& app) = nullptr;

    /// @brief Представляет различные этапы жизненного цикла приложения.
    enum Stage {
        /// @brief Приложение находится в неинициализированном состоянии.
        Uninitialized,
        /// @brief Приложение в данный момент загружается.
        Booting,
        /// @brief Приложение завершило процесс загрузки и готово к инициализации.
        BootComplete,
        /// @brief Приложение в данный момент инициализируется.
        Initializing,
        /// @brief Инициализация приложения завершена.
        Initialized,
        /// @brief Приложение в данный момент работает.
        Running,
        /// @brief Приложение находится в процессе завершения работы.
        ShuttingDown
    };

    /// @brief Стадия выполнения приложения.
    Stage stage;

    Application() :
    AppConfig(),
    engine(nullptr),
    state(nullptr),
    StateMemoryRequirement(),
    FrameAllocator(),
    WorldGeometries(),
    RendererLibrary(),
    GameLibrary(),
    Boot(nullptr),
    Initialize(nullptr),
    Update(nullptr),
    Render(nullptr),
    OnResize(nullptr),
    Shutdown(nullptr),
    LibOnLoad(nullptr),
    LibOnUnload(nullptr),
    stage() {}
};
