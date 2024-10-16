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

struct ApplicationState {
    LinearAllocator SystemAllocator;
    //MMemory* mem;
    Logger* logger;
    //Input Inputs;
    Event* Events ;
    bool IsRunning;
    bool IsSuspended;
    MWindow* Window;
    class Renderer* Render;
    class GameTypes* GameInst;

    //Системы
    //TextureSystem* TexSys;
    class JobSystem* JobSystemInst;
    class RenderViewSystem* RenderViewSystemInst;
    class ResourceSystem* ResourceSystemInst;

    Metrics metrics;
    
    u32 width;
    u32 height;
    Clock clock;
    f64 LastTime;
};

class Application
{
public:
    MAPI static ApplicationState* State; //ЗАДАЧА: убрать MAPI 
public:
    Application() = default;
    ~Application() {}

    MAPI bool Create(GameTypes* GameInst);

    MAPI bool ApplicationRun();

    static void ApplicationGetFramebufferSize(u32& width, u32& height);

    MAPI void* operator new(u64 size);
    MAPI void operator delete(void* ptr);

private:
    // Обработчики событий
    static bool OnEvent(u16 code, void* sender, void* ListenerInst, EventContext context);
    static bool OnResized(u16 code, void* sender, void* ListenerInst, EventContext context);
};
