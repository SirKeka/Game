#pragma once

#include "defines.hpp"

#include "logger.hpp"
#include "platform/platform.hpp"
#include "core/mmemory.hpp"
#include "event.hpp"
#include "input.hpp"
#include "clock.hpp"

class GameTypes;
class Renderer;
class TextureSystem;

struct ApplicationState {
    LinearAllocator SystemAllocator;
    MMemory* mem;
    Logger* logger;
    //Input Inputs;
    //Event Events;
    bool IsRunning;
    bool IsSuspended;
    MWindow* Window;
    Renderer* Render;
    GameTypes* GameInst;

    //Системы
    TextureSystem* TexSys;
    
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
    static void* AllocMemory(u64 size);

private:
    // Обработчики событий
    static bool OnEvent(u16 code, void* sender, void* ListenerInst, EventContext context);
    static bool OnKey(u16 code, void* sender, void* ListenerInst, EventContext context);
    static bool OnResized(u16 code, void* sender, void* ListenerInst, EventContext context);

};
