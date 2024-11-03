#pragma once

#include "defines.hpp"

#include "platform/platform.hpp"
// Основные компоненты ядра
#include "clock.hpp"
#include "event.hpp"
#include "input.hpp"
#include "logger.hpp"
#include "metrics.hpp"
#include "mmemory.hpp"
// Ресурсы
#include "resources/mesh.hpp"
#include "resources/skybox.hpp"
#include "resources/ui_text.hpp"

class Engine
{
    LinearAllocator SystemAllocator;
    //MMemory* mem;
    Logger* logger;
    //Input Inputs;
    Event* Events ;
    bool IsRunning;
    bool IsSuspended;
    MWindow* Window;
    class Renderer* Render;
    class Application* GameInst;

    //Системы
    //TextureSystem* TexSys;
    class JobSystem* JobSystemInst;
    class RenderViewSystem* RenderViewSystemInst;
    class ResourceSystem* ResourceSystemInst;

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

private:
    // Обработчики событий
    static bool OnEvent(u16 code, void* sender, void* ListenerInst, EventContext context);
    static bool OnResized(u16 code, void* sender, void* ListenerInst, EventContext context);
};
