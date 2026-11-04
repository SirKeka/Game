#pragma once

#include "clock.h"

class MAPI Metrics
{
    constexpr static u8 AVG_COUNT = 30;

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
    
    static Metrics* instance;

    constexpr Metrics() : FrameAvgCounter(), MsTimes(), MsAvg(), frames(), DrawCalls(), AccumulatedFrameMs(), fps(), function() {}
public:

    Metrics(const Metrics&) = delete;
    Metrics& operator= (const Metrics&) = delete;

    static void Initialize();
    static Metrics* GetInstance();

    /// @brief Обновляет метрики; следует вызывать один раз за кадр.
    /// @param FrameElapsedTime Количество времени, прошедшее с предыдущего кадра.
    /// @return 
    void Update(f64 FrameElapsedTime);

    /// @return текущее среднее количество кадров в секунду (fps).
    const f64& FPS() { return fps; }

    /// @return текущее среднее время кадра в миллисекундах.
    const f64& FrameTime() { return MsAvg; }

    /// @brief Получает как текущее среднее количество кадров в секунду (fps), так и время кадра в миллисекундах.
    /// @param OutFPS Ссылка на переменную для хранения текущего среднего количества кадров в секунду (fps).
    /// @param OutFrameMs Ссылка на переменную для хранения текущего среднего времени кадра в миллисекундах.
    void Frame(f64& OutFPS, f64& OutFrameMs);

    u32& GetDrawCallsCounter() { return DrawCalls; }

    void BeginFunction(const char* FunctionName);
    void EndFunction(const char* FunctionName);
    f64 GetFunctionExecutionTime(const char* FunctionName);

    void* operator new(u64 size);
}; // namespace Metrics
