#pragma once

#include "defines.hpp"

#include "platform/platform.hpp"
// Основные компоненты ядра
#include "clock.hpp"
// #include "event.hpp"
// #include "logger.hpp"
#include "metrics.hpp"
// #include "mmemory.hpp"
#include "systems_manager.hpp"

#include "systems/font_system.hpp"
// Ресурсы
#include "resources/mesh.hpp"
#include "resources/skybox.hpp"
#include "resources/ui_text.hpp"
#include "renderer/views/render_view.hpp"

/// @brief Конфигурация приложения.
struct ApplicationConfig {
    i16 StartPosX;                            // Начальное положение окна по оси X, если применимо.
    i16 StartPosY;                            // Начальное положение окна по оси Y, если применимо.
    i16 StartWidth;                           // Начальная ширина окна, если применимо.
    i16 StartHeight;                          // Начальная высота окна, если применимо.
    const char* name;                         // Имя приложения, используемое в оконном режиме, если применимо.
    FontSystemConfig FontConfig;              // Конфигурация для системы шрифтов.
    DArray<RenderView::Config> RenderViews{}; // Массив конфигураций представления рендеринга.
};

class Engine
{
    LinearAllocator SystemAllocator;
    
    bool IsRunning;
    bool IsSuspended;
    // WindowSystem* Window;
    //class RendererSystem* Render;
    class Application* GameInst;

    //Системы
    //TextureSystem* TexSys;
    class RenderViewSystem* RenderViewSystemInst;
    class ResourceSystem* ResourceSystemInst;

    SystemsManager SysManager;

    // Metrics metrics;
    
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
