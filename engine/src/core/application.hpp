#pragma once

#include "defines.hpp"

#include "platform/platform.hpp"
#include "core/mmemory.hpp"
#include "core/event.hpp"
#include "input.hpp"
#include "clock.hpp"

#include "memory/linear_allocator.hpp"

#include "renderer/renderer.hpp"

class Game;

struct ApplicationState {
    Input* Inputs;
    Event* Events;
    bool IsRunning;
    bool IsSuspended;
    MWindow* Window;
    Renderer Render;
    Game* GameInst;
    
    u32 width;
    u32 height;
    Clock clock;
    f64 LastTime;
    LinearAllocator SystemAllocator;
};

class Application
{
private:
    static ApplicationState* AppState;
public:
    Application() = default;
    ~Application();

    MAPI bool ApplicationCreate(Game* GameInst);

    MAPI bool ApplicationRun();

    void ApplicationGetFramebufferSize(u32& width, u32& height);

    MAPI void* operator new(u64 size);
    MAPI void operator delete(void* ptr);

private:
    // Обработчики событий
    static bool ApplicationOnEvent(u16 code, void* sender, void* ListenerInst, EventContext context);
    static bool ApplicationOnKey(u16 code, void* sender, void* ListenerInst, EventContext context);
    static bool ApplicationOnResized(u16 code, void* sender, void* ListenerInst, EventContext context);

    
};
