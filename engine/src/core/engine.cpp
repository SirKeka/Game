#include "engine.hpp"
#include "version.hpp"

//#include "memory/linear_allocator.hpp"
#include "application_types.hpp"
#include "renderer/rendering_system.hpp"

#include "console.hpp"
#include "mvar.hpp"
#include "input.hpp"
#include "metrics.hpp"

// Системы
#include "systems/texture_system.hpp"
#include "systems/material_system.hpp"
#include "systems/geometry_system.hpp"
#include "systems/resource_system.hpp"
#include "systems/shader_system.hpp"
#include "systems/camera_system.hpp"
#include "systems/render_view_system.hpp"
#include "systems/job_systems.hpp"
#include "systems/font_system.hpp"
#include "platform/platform.hpp"

#include "renderer/views/render_view_pick.hpp"

// Компоненты ядра движка
#include "uuid.hpp"

#include <new>

Engine* Engine::pEngine = nullptr;

constexpr Engine::Engine() 
: 
SystemAllocator(),
IsRunning(),
IsSuspended(),
GameInst(),
SysManager(),
width(),
height(),
clock(),
LastTime() {}

bool Engine::Create(Application& GameInst)
{
    if (GameInst.engine) {
        MERROR("Engine::Create вызывался более одного раза.");
        return false;
    }

    // Начальный генератор uuid.
    // ЗАДАЧА: Добавить более лучший начальный генератор.
    uuid::Seed(101);

    Metrics::Initialize();

    GameInst.engine = new Engine();
    pEngine = GameInst.engine;
    pEngine->GameInst = &GameInst;

    // Инициализируйте подсистемы.
    if (!SystemsManager::Initialize(&pEngine->SysManager, GameInst.AppConfig)) {
        MFATAL("Системный менеджер не смог инициализироваться. Процесс прерывается.");
        return false;
    }

    GameInst.stage = Application::Stage::Booting;
    if (!GameInst.Boot(GameInst)) {
        MFATAL("Game::Boot - Сбой последовательности загрузки игры; прерывание работы приложения.");
        return false;
    } GameInst.stage = Application::Stage::BootComplete;

    if (!pEngine->SysManager.PostBootInitialize(GameInst.AppConfig)) {
        MFATAL("Инициализация системного менеджера после загрузки не удалась!")
        return false;
    }

    // Сообщить о версии движка
    MINFO("Moon Engine v. %s", MVERSION);

    // Инициализируйте игру.
    GameInst.stage = Application::Stage::Initializing;
    if (!GameInst.Initialize(GameInst)) {
        MFATAL("Не удалось инициализировать игру.");
        return false;
    } GameInst.stage = Application::Stage::Initialized;

    // Вызовите resize один раз, чтобы убедиться, что установлен правильный размер.
    RenderingSystem::OnResized(pEngine->width, pEngine->height);
    GameInst.OnResize(GameInst, pEngine->width, pEngine->height);

    return true;
}

bool Engine::Run() {
    GameInst->stage = Application::Stage::Running;
    IsRunning = true;
    clock.Start();
    clock.Update();
    LastTime = clock.elapsed;
    // f64 RunningTime = 0;
    [[maybe_unused]]u8 FrameCount = 0;      // 
    f64 TargetFrameSeconds = 1.0f / 60;
    f64 FrameElapsedTime = 0;

    MINFO(MemorySystem::GetMemoryUsageStr().c_str());

    while (IsRunning) {
        if(!WindowSystem::Messages()) {
            IsRunning = false;
        }

        if(!IsSuspended) {
            // Обновите часы и получите разницу во времени.
            clock.Update();
            f64 CurrentTime = clock.elapsed;
            f64 delta = (CurrentTime - LastTime);
            f64 FrameStartTime = WindowSystem::PlatformGetAbsoluteTime();

            // Обновлление системы работы(потоков)
            SysManager.Update(delta);

            //Обновление метрик(статистики)
            Metrics::Update(FrameElapsedTime);

            if (!GameInst->Update(*GameInst, delta)) {
                MFATAL("Ошибка обновления игры, выключение.");
                IsRunning = false;
                break;
            }

            RenderPacket packet = {};
            packet.DeltaTime = delta;

            // Вызовите процедуру рендеринга игры.
            if (!GameInst->Render(*GameInst, packet, delta)) {
                MFATAL("Ошибка рендеринга игры, выключение.");
                IsRunning = false;
                break;
            }

            RenderingSystem::DrawFrame(packet);

            // Выясните, сколько времени занял кадр и, если ниже
            f64 FrameEndTime = WindowSystem::PlatformGetAbsoluteTime();
            FrameElapsedTime = FrameEndTime - FrameStartTime;
            // RunningTime += FrameElapsedTime;
            f64 RemainingSeconds = TargetFrameSeconds - FrameElapsedTime;

            if (RemainingSeconds > 0) {
                u64 Remaining_ms = (RemainingSeconds * 1000);

                // Если осталось время, верните его ОС.
                bool LimitFrames = false;
                if (Remaining_ms > 0 && LimitFrames) {
                    PlatformSleep(Remaining_ms - 1);
                }

                FrameCount++;
            }

            // ПРИМЕЧАНИЕ. Обновление/копирование состояния ввода всегда
            // должно выполняться после записи любого ввода; т.е. перед этой
            // строкой. В целях безопасности входные данные обновляются в
            // последнюю очередь перед завершением этого кадра.
            InputSystem::Update(delta);

            // Update last time
            LastTime = CurrentTime;
        }
    }

    IsRunning = false;

    // Выключение игры
    GameInst->stage = Application::Stage::ShuttingDown;
    GameInst->Shutdown(*GameInst);
    // Отключение системы событий.
    EventSystem::Unregister(EventSystem::ApplicationQuit, nullptr, OnEvent);

    pEngine->SysManager.~SystemsManager();
    GameInst->stage = Application::Stage::Uninitialized;

    return true;
}

void *Engine::operator new(u64 size)
{
    return MemorySystem::Allocate(size, Memory::Engine);
}

void Engine::operator delete(void *ptr, u64 size)
{
    return MemorySystem::Free(ptr, size, Memory::Engine);
}

bool Engine::OnEvent(u16 code, void *sender, void *ListenerInst, EventContext context)
{
    switch (code) {
        case EventSystem::ApplicationQuit: {
            MINFO("Получено событие EventSystem::ApplicationQuit, завершение работы.\n");
            pEngine->IsRunning = false;
            return true;
        }
    }

    return false;
}

bool Engine::OnResized(u16 code, void *sender, void *ListenerInst, EventContext context)
{
    if (code == EventSystem::Resized) {
        u16 width = context.data.u16[0];
        u16 height = context.data.u16[1];

        // Проверьте, отличается ли это. Если да, запустите событие изменения размера.
        if (width != pEngine->width || height != pEngine->height) {
            pEngine->width = width;
            pEngine->height = height;

            MDEBUG("Изменение размера окна: %i, %i", width, height);

            // Обработка сворачивания
            if (width == 0 || height == 0) {
                MINFO("Окно свернуто, приложение приостанавливается.");
                pEngine->IsSuspended = true;
                return true;
            } else {
                if (pEngine->IsSuspended) {
                    MINFO("Окно восстановлено, возобновляется приложение.");
                    pEngine->IsSuspended = false;
                }
                pEngine->GameInst->OnResize(*pEngine->GameInst, width, height);
                //pEngine->Render->OnResized(width, height);
                return true;
            }
        }
    }
    // Событие намеренно не обрабатывается, чтобы другие слушатели могли его получить.
    return false;
}

void Engine::OnEventSystemInitialized()
{
    EventSystem::Register(EventSystem::ApplicationQuit, nullptr, OnEvent);
    EventSystem::Register(EventSystem::Resized, nullptr, OnResized);
    // EventSystem::Register(EVENT_CODE_OBJECT_HOVER_ID_CHANGED, nullptr, OnEvent);
}
