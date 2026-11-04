#include "metrics.h"
#include "core/memory_system.h"

/*struct sMetrics
{
    u8 FrameAvgCounter;
    f64 MsTimes[AVG_COUNT];
    f64 MsAvg;
    i32 frames;
    u32 DrawCalls;
    f64 AccumulatedFrameMs;
    f64 fps;

    struct Function {
        // ЗАДАЧА: Добавить массив или словарь для отслеживания времени нескольких функций
        const char* str; 
        Clock timer;
    } function; 

    /// @brief Инициализирует систему метрик.
    constexpr sMetrics() : FrameAvgCounter(), MsTimes(), MsAvg(), frames(), DrawCalls(), AccumulatedFrameMs(), fps(), function() {}

    void* operator new(u64 size) {
        return MemorySystem::Allocate(size, Memory::Engine);
    }
};*/

Metrics *Metrics::instance = nullptr;

void Metrics::Initialize()
{
    if (!instance) {
        instance = new Metrics();
    }
}

Metrics *Metrics::GetInstance()
{
    return instance;
}

void Metrics::Update(f64 FrameElapsedTime)
{
    // Рассчитать среднее значение кадра в мс
    f64 FrameMs = (FrameElapsedTime * 1000.0);
    MsTimes[FrameAvgCounter] = FrameMs;
    if (FrameAvgCounter == AVG_COUNT - 1) {
        for (u8 i = 0; i < AVG_COUNT; ++i) {
            MsAvg += MsTimes[i];
        }

        MsAvg /= AVG_COUNT;
    }
    FrameAvgCounter++;
    FrameAvgCounter %= AVG_COUNT;

    // Рассчитать кадры в секунду.
    AccumulatedFrameMs += FrameMs;
    if (AccumulatedFrameMs > 1000) {
        fps = frames;
        AccumulatedFrameMs -= 1000;
        frames = 0;
    }

    // Подсчитать все кадры.
    frames++;
}

void Metrics::Frame(f64 &OutFPS, f64 &OutFrameMs)
{
    OutFPS = fps;
    OutFrameMs = MsAvg;
}

void Metrics::BeginFunction(const char *FunctionName)
{
    function.str = FunctionName;
    function.timer.Start();
}

void Metrics::EndFunction(const char *FunctionName)
{
    if (MString::Compare(function.str, FunctionName)) {
        function.timer.Update();
    }
}

f64 Metrics::GetFunctionExecutionTime(const char *FunctionName)
{
    if (MString::Compare(function.str, FunctionName)) {
        return function.timer.elapsed;
    }
    return 0;
}

void *Metrics::operator new(u64 size)
{
    return MemorySystem::Allocate(size, Memory::Engine);
}
