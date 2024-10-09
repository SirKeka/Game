#pragma once

#include "defines.hpp"

constexpr u8 AVG_COUNT = 30;

class MAPI Metrics
{
private:
    u8 FrameAvgCounter;
    f64 MsTimes[AVG_COUNT];
    f64 MsAvg;
    i32 frames;
    f64 AccumulatedFrameMs;
    f64 fps;
public:
    /// @brief Инициализирует систему метрик.
    constexpr Metrics() : FrameAvgCounter(), MsTimes(), MsAvg(), frames(), AccumulatedFrameMs(), fps() {}
    ~Metrics() = default;

    /// @brief Обновляет метрики; следует вызывать один раз за кадр.
    /// @param FrameElapsedTime Количество времени, прошедшее с предыдущего кадра.
    /// @return 
    void Update(f64 FrameElapsedTime);

    /// @return текущее среднее количество кадров в секунду (fps).
    const f64& FPS();

    /// @return текущее среднее время кадра в миллисекундах.
    const f64& FrameTime();

    /// @brief Получает как текущее среднее количество кадров в секунду (fps), так и время кадра в миллисекундах.
    /// @param OutFPS Ссылка на переменную для хранения текущего среднего количества кадров в секунду (fps).
    /// @param OutFrameMs Ссылка на переменную для хранения текущего среднего времени кадра в миллисекундах.
    void Frame(f64& OutFPS, f64& OutFrameMs);
};
