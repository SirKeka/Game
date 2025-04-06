#pragma once

#include "defines.hpp"

// Основные компоненты ядра
#include "clock.hpp"
#include "systems_manager.hpp"

#include "systems/font_system.h"

#include "platform/platform.hpp"
#include "renderer/renderer_types.h"

#include "frame_data.h"

/// @brief Конфигурация приложения.
/// @param i16_StartPosX начальное положение окна по оси X, если применимо.
/// @param i16_StartPosY начальное положение окна по оси Y, если применимо.
/// @param i16_StartWidth начальная ширина окна, если применимо.
/// @param i16_StartHeight начальная высота окна, если применимо.
/// @param const_char*_name имя приложения, используемое в оконном режиме, если применимо.
/// @param FontSystemConfig_FontConfig конфигурация для системы шрифтов.
/// @param DArray<RenderView::Config>_RenderViews массив конфигураций представления рендеринга.
/// @param u64_FrameAllocatorSize размер распределителя кадров.
/// @param u64_AppFrameDataSize размер данных кадра, специфичных для приложения. Установите на 0, если не используется.
struct ApplicationConfig {
    i16 StartPosX;
    i16 StartPosY;
    i16 StartWidth;
    i16 StartHeight;
    const char* name;
    FontSystemConfig FontConfig;
    DArray<RenderView::Config> RenderViews;
    RendererPlugin* plugin;
    u64 FrameAllocatorSize;
    u64 AppFrameDataSize;
};

template class DArray<RenderView::Config>;

class Engine
{
    LinearAllocator SystemAllocator;
    
    bool IsRunning;
    bool IsSuspended;

    struct Application* GameInst;

    SystemsManager SysManager;
    
    u32 width;
    u32 height;
    Clock clock;
    f64 LastTime;

    LinearAllocator FrameAllocator; // Распределитель, используемый для покадрового распределения, который сбрасывается каждый кадр.

    FrameData pFrameData;

    MAPI static Engine* pEngine; //ЗАДАЧА: убрать MAPI 

    constexpr Engine(const ApplicationConfig& config);
public:
    ~Engine() {}
    Engine(const Engine&) = delete;
    Engine& operator= (const Engine&) = delete;

    MAPI static bool Create(Application& GameInst);

    MAPI bool Run();

    MAPI void* operator new(u64 size);
    MAPI void operator delete(void* ptr, u64 size);
    static void OnEventSystemInitialized();

    MAPI const FrameData& GetFrameData() const;

private:
    // Обработчики событий
    static bool OnEvent(u16 code, void* sender, void* ListenerInst, EventContext context);
    static bool OnResized(u16 code, void* sender, void* ListenerInst, EventContext context);
};
