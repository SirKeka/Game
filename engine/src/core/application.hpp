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
#include "resources/ui_text.hpp"

struct ApplicationState {
    //LinearAllocator SystemAllocator;
    //MMemory* mem;
    Logger* logger           {nullptr};
    //Input Inputs;
    Event* Events            {nullptr};
    bool IsRunning             {false};
    bool IsSuspended           {false};
    MWindow* Window          {nullptr};
    class Renderer* Render   {nullptr};
    class GameTypes* GameInst{nullptr};

    //Системы
    //TextureSystem* TexSys;
    class JobSystem* JobSystemInst              {nullptr};
    class RenderViewSystem* RenderViewSystemInst{nullptr};
    class ResourceSystem* ResourceSystemInst    {nullptr};
    
    u32 width   {};
    u32 height  {};
    Clock clock {};
    f64 LastTime{};

    // ЗАДАЧА: временно
    Skybox sb;

    Mesh meshes[10]        {};
    Mesh* CarMesh   {nullptr};
    Mesh* SponzaMesh{nullptr};
    bool ModelsLoaded {false};
    Mesh UIMeshes[10]      {};
    Text TestText          {};
    Text TestSysText       {};
    u32 HoveredObjectID    {};    // Уникальный идентификатор объекта, на который в данный момент наведен курсор.
    // ЗАДАЧА: временно
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
    static bool OnKey(u16 code, void* sender, void* ListenerInst, EventContext context);
    static bool OnResized(u16 code, void* sender, void* ListenerInst, EventContext context);

    //ЗАДАЧА: временно
    static bool OnDebugEvent(u16 code, void* sender, void* ListenerInst, EventContext context);
    //ЗАДАЧА: временно
};
