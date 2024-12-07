#pragma once

#include "defines.hpp"

// Основные компоненты ядра
#include "clock.hpp"
#include "systems_manager.hpp"

#include "systems/font_system.hpp"
// Ресурсы
#include "renderer/renderer_types.hpp"

/// @brief Конфигурация приложения.
struct ApplicationConfig {
    /// @brief Начальное положение окна по оси X, если применимо.
    i16 StartPosX;
    /// @brief Начальное положение окна по оси Y, если применимо.
    i16 StartPosY;
    /// @brief Начальная ширина окна, если применимо.
    i16 StartWidth;
    /// @brief Начальная высота окна, если применимо.
    i16 StartHeight;
    /// @brief Имя приложения, используемое в оконном режиме, если применимо.
    const char* name;
    /// @brief Конфигурация для системы шрифтов.
    FontSystemConfig FontConfig;
    /// @brief Массив конфигураций представления рендеринга.
    DArray<RenderView::Config> RenderViews{};

    RendererPlugin* RenderPlugin;
};

class Engine
{
    LinearAllocator SystemAllocator;
    
    bool IsRunning;
    bool IsSuspended;

    class Application* GameInst;

    SystemsManager SysManager;
    
    u32 width;
    u32 height;
    Clock clock;
    f64 LastTime;

    MAPI static Engine* pEngine; //ЗАДАЧА: убрать MAPI 

    constexpr Engine();
public:
    ~Engine() {}
    Engine(const Engine&) = delete;
    Engine& operator= (const Engine&) = delete;

    MAPI static bool Create(Application* GameInst);

    MAPI bool Run();

    MAPI void* operator new(u64 size);
    MAPI void operator delete(void* ptr, u64 size);
    static void OnEventSystemInitialized();

private:
    // Обработчики событий
    static bool OnEvent(u16 code, void* sender, void* ListenerInst, EventContext context);
    static bool OnResized(u16 code, void* sender, void* ListenerInst, EventContext context);
};
