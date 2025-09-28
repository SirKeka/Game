#include "metrics.h"
#include "core/memory_system.h"
#include "clock.h"

constexpr u8 AVG_COUNT = 30;

struct sMetrics
{
    u8 FrameAvgCounter;
    f64 MsTimes[AVG_COUNT];
    f64 MsAvg;
    i32 frames;
    f64 AccumulatedFrameMs;
    f64 fps;

    struct Function {
        // ЗАДАЧА: Добавить массив или словарь для отслеживания времени нескольких функций
        const char* str; 
        Clock timer;
    } function; 

    /// @brief Инициализирует систему метрик.
    constexpr sMetrics() : FrameAvgCounter(), MsTimes(), MsAvg(), frames(), AccumulatedFrameMs(), fps(), function() {}

    void* operator new(u64 size) {
        return MemorySystem::Allocate(size, Memory::Engine);
    }
};

static sMetrics* pMetrics = nullptr;

void Metrics::Initialize()
{
    pMetrics = new sMetrics();
}

void Metrics::Update(f64 FrameElapsedTime)
{
    // Рассчитать среднее значение кадра в мс
    f64 FrameMs = (FrameElapsedTime * 1000.0);
    pMetrics->MsTimes[pMetrics->FrameAvgCounter] = FrameMs;
    if (pMetrics->FrameAvgCounter == AVG_COUNT - 1) {
        for (u8 i = 0; i < AVG_COUNT; ++i) {
            pMetrics->MsAvg += pMetrics->MsTimes[i];
        }

        pMetrics->MsAvg /= AVG_COUNT;
    }
    pMetrics->FrameAvgCounter++;
    pMetrics->FrameAvgCounter %= AVG_COUNT;

    // Рассчитать кадры в секунду.
    pMetrics->AccumulatedFrameMs += FrameMs;
    if (pMetrics->AccumulatedFrameMs > 1000) {
        pMetrics->fps = pMetrics->frames;
        pMetrics->AccumulatedFrameMs -= 1000;
        pMetrics->frames = 0;
    }

    // Подсчитать все кадры.
    pMetrics->frames++;
}

const f64& Metrics::FPS()
{
    return pMetrics->fps;
}

const f64& Metrics::FrameTime()
{
    return pMetrics->MsAvg;
}

void Metrics::Frame(f64 &OutFPS, f64 &OutFrameMs)
{
    OutFPS = pMetrics->fps;
    OutFrameMs = pMetrics->MsAvg;
}

void Metrics::BeginFunction(const char *FunctionName)
{
    if (pMetrics) {
        pMetrics->function.str = FunctionName;
        pMetrics->function.timer.Start();
    }
}

void Metrics::EndFunction(const char *FunctionName)
{
    if (pMetrics && MString::Compare(pMetrics->function.str, FunctionName)) {
        pMetrics->function.timer.Update();
    }
}

f64 Metrics::GetFunctionExecutionTime(const char *FunctionName)
{
    if (MString::Compare(pMetrics->function.str, FunctionName)) {
        return pMetrics->function.timer.elapsed;
    }
    return 0;
}
