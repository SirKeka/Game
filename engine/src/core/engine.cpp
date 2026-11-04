#include "engine.h"

#include "application_types.h"
#include "renderer/rendering_system.h"

#include "event.h"
#include "mvar.h"
#include "input.h"
#include "metrics.h"

// Системы
#include "systems/texture_system.h"
#include "systems/material_system.h"
#include "systems/geometry_system.h"
#include "systems/resource_system.h"
#include "systems/camera_system.hpp"
#include "systems/job_systems.h"
#include "systems/font_system.h"
#include "platform/platform.hpp"

// Компоненты ядра движка
#include "uuid.hpp"

#include <new>

Engine* Engine::pEngine = nullptr;

constexpr Engine::Engine(const ApplicationConfig& config) 
: 
SystemAllocator(),
IsRunning(),
IsSuspended(),
GameInst(nullptr),
SysManager(),
width(),
height(),
clock(),
LastTime(),
resizing(),
FramesSinceResize(),
FrameAllocator(config.FrameAllocatorSize),
frameData() {}

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

    GameInst.engine = new Engine(GameInst.AppConfig);
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

    // Настройте распределитель кадров.
    pEngine->frameData.FrameAllocator = &pEngine->FrameAllocator;
    if (GameInst.AppConfig.AppFrameDataSize > 0) {
        pEngine->frameData.ApplicationFrameData = MemorySystem::Allocate(GameInst.AppConfig.AppFrameDataSize, Memory::Game, true);
    } else {
        pEngine->frameData.ApplicationFrameData = nullptr;
    }

    if (!pEngine->SysManager.PostBootInitialize(GameInst.AppConfig)) {
        MFATAL("Инициализация системного менеджера после загрузки не удалась!")
        return false;
    }

    // Инициализируйте игру.
    GameInst.stage = Application::Stage::Initializing;
    if (!GameInst.Initialize(GameInst)) {
        MFATAL("Не удалось инициализировать игру.");
        return false;
    } GameInst.stage = Application::Stage::Initialized;

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
    f64 TargetFrameSeconds = 1.F / 60;
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

            frameData.TotalTime = CurrentTime;
            frameData.DeltaTime = (f32)delta;

            // Сброс распределителя кадров
            FrameAllocator.FreeAll(false);

            // Обновлление системы работы(потоков)
            SysManager.Update(frameData);

            //Обновление метрик(статистики)
            Metrics::GetInstance()->Update(FrameElapsedTime);
            auto renderer = SysManager.GetRenderingSystem();

            // Убедитесь, что размер окна в данный момент не изменяется, подождав указанное количество кадров после последней операции изменения размера, прежде чем выполнять внутренние обновления.
            if (pEngine->resizing) {
                pEngine->FramesSinceResize++;

                // Если с момента изменения размера прошло необходимое количество кадров, выполните сами обновления.
                if (pEngine->FramesSinceResize >= 30) {
                    renderer->OnResized(pEngine->width, pEngine->height);

                    // ПРИМЕЧАНИЕ: Не проверяйте результат, так как это, скорее всего, приведёт к пересозданию цепочки обмена и загрузке следующего кадра.
                    renderer->PrepareFrame(pEngine->frameData);

                    // Уведомите приложение об изменении размера.
                    pEngine->GameInst->OnResize(*pEngine->GameInst, pEngine->width, pEngine->height);

                    pEngine->FramesSinceResize = 0;
                    pEngine->resizing = false;
                } else {
                    // Пропустите рендеринг кадра и повторите попытку в следующий раз.
                    // ПРИМЕЧАНИЕ: Имитируйте «отрисовку» кадра с частотой 60 кадров в секунду.
                    PlatformSleep(16);
                }

                // В любом случае, не обрабатывайте этот кадр дальше при изменении размера.
                // Попробуйте снова на следующем кадре.
                continue;
            }
            if (!renderer->PrepareFrame(pEngine->frameData)) {
                /* Это может произойти не только из-за изменения размера, 
                 * описанного выше, но и при изменении флага рендерера (например, VSync), 
                 * что также может потребовать перераспределения ресурсов. 
                 * Для обработки этой ситуации уведомите приложение о событии изменения размера, 
                 * которое оно затем сможет передать своим рендер-графам по мере необходимости.
                */
                pEngine->GameInst->OnResize(*pEngine->GameInst, pEngine->width, pEngine->height);
                continue;
            }

            if (!GameInst->Update(*GameInst, frameData)) {
                MFATAL("Ошибка обновления игры, выключение.");
                IsRunning = false;
                break;
            }

            if (!renderer->Begin(frameData)) {
                MFATAL("Не удалось запустить модуль отрисовки. Завершение работы.");
                IsRunning = false;
                break;
            }

            // Начать группировку событий рендеринга «PrepareFrame».
            renderer->BeginDebugLabel("prepare_frame", FVec3(1.F, 1.F, 0.F));

            SysManager.RendererFramePrepare(frameData);

            // Позвольте приложению сгенерировать пакет рендеринга.
            bool PrepareResult = pEngine->GameInst->PrepareFrame(pEngine->GameInst, pEngine->frameData);
            // Завершить группировку событий рендеринга «PrepareFrame».
            renderer->EndDebugLabel();
            if (!PrepareResult) {
                continue;
            }

            // Вызовите процедуру рендеринга игры.
            if (!GameInst->RenderFrame(*GameInst, frameData)) {
                MFATAL("Ошибка рендеринга игры, выключение.");
                IsRunning = false;
                break;
            }

            // Завершите кадр.
            renderer->End(frameData);

            // Представьте кадр.
            if (!renderer->Present(frameData)) {
                MERROR("Вызов renderer_present завершился с ошибкой. Вероятно, это неустранимая ошибка. Завершение работы.");
                IsRunning = false;
                break;
            }

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

bool Engine::OnEvent(u16 code, void *sender, void *ListenerInst, const EventContext& context)
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

bool Engine::OnResized(u16 code, void *sender, void *ListenerInst, const EventContext& context)
{
    if (code == EventSystem::Resized) {
        // Отметить как изменение размера и сохранить изменение, но дождаться регенерации.
        pEngine->resizing = true;
        // Также сбросить количество кадров с момента последней операции изменения размера.
        pEngine->FramesSinceResize = 0;

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

const FrameData &Engine::GetFrameData() const
{
    return frameData;
}
