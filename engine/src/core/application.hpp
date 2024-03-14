#pragma once

#include "defines.hpp"

#include "logger.hpp"
#include "platform/platform.hpp"
#include "core/mmemory.hpp"
#include "core/event.hpp"
#include "input.hpp"
#include "clock.hpp"

#include "memory/linear_allocator.hpp"

#include "renderer/renderer.hpp"

class GameTypes;

struct ApplicationState {
    LinearAllocator SystemAllocator;
    MMemory* mem;
    Logger* logger;
    Input* Inputs;
    Event* Events;
    bool IsRunning;
    bool IsSuspended;
    MWindow* Window;
    Renderer* Render;
    GameTypes* GameInst;
    
    u32 width;
    u32 height;
    Clock clock{};
    f64 LastTime;

};

class Application
{
public:
    MAPI static ApplicationState* AppState; //TODO: убрать MAPI 
public:
    Application() = default;
    ~Application();

    MAPI bool ApplicationCreate(GameTypes* GameInst);

    MAPI bool ApplicationRun();

    static void ApplicationGetFramebufferSize(u32& width, u32& height);

    MAPI void* operator new(u64 size);
    MAPI void operator delete(void* ptr);

private:
    // Обработчики событий
    static bool ApplicationOnEvent(u16 code, void* sender, void* ListenerInst, EventContext context);
    static bool ApplicationOnKey(u16 code, void* sender, void* ListenerInst, EventContext context);
    static bool ApplicationOnResized(u16 code, void* sender, void* ListenerInst, EventContext context);

    
};
