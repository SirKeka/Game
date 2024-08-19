#pragma once

#include "defines.hpp"

#include "logger.hpp"
#include "platform/platform.hpp"
#include "core/mmemory.hpp"
#include "event.hpp"
#include "input.hpp"
#include "clock.hpp"
#include "resources/mesh.hpp"
#include "resources/skybox.hpp"

struct ApplicationState {
    //LinearAllocator SystemAllocator;
    //MMemory* mem;
    Logger* logger;
    //Input Inputs;
    //Event Events;
    bool IsRunning;
    bool IsSuspended;
    MWindow* Window;
    class Renderer* Render;
    class GameTypes* GameInst;

    //Системы
    //TextureSystem* TexSys;
    class JobSystem* JobSystemInst{nullptr};
    class RenderViewSystem* RenderViewSystemInst{nullptr};
    class ResourceSystem* ResourceSystemInst{nullptr};
    
    u32 width;
    u32 height;
    Clock clock{};
    f64 LastTime;

    // ЗАДАЧА: временно
    Skybox sb;

    Mesh meshes[10];
    u32 MeshCount{};

    Mesh UIMeshes[10];
    u32 UIMeshCount;
    // ЗАДАЧА: временно

};

class Application
{
public:
    MAPI static ApplicationState* State; //ЗАДАЧА: убрать MAPI 
public:
    Application() = default;
    ~Application() {}

    MAPI bool ApplicationCreate(GameTypes* GameInst);

    MAPI bool ApplicationRun();

    static void ApplicationGetFramebufferSize(u32& width, u32& height);

    MAPI void* operator new(u64 size);
    MAPI void operator delete(void* ptr);

private:
    // Обработчики событий
    static bool OnEvent(u16 code, void* sender, void* ListenerInst, EventContext context);
    static bool OnKey(u16 code, void* sender, void* ListenerInst, EventContext context);
    static bool OnResized(u16 code, void* sender, void* ListenerInst, EventContext context);

    //ЗАДАЧА: временно
    static bool OnDebugEvent(u16 code, void* sender, void* ListenerInst, EventContext context);
    //ЗАДАЧА: временно
};
