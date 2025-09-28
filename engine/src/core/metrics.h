#pragma once

#include "defines.h"

namespace Metrics
{
    void Initialize();

    /// @brief Обновляет метрики; следует вызывать один раз за кадр.
    /// @param FrameElapsedTime Количество времени, прошедшее с предыдущего кадра.
    /// @return 
    MAPI void Update(f64 FrameElapsedTime);

    /// @return текущее среднее количество кадров в секунду (fps).
    MAPI const f64& FPS();

    /// @return текущее среднее время кадра в миллисекундах.
    MAPI const f64& FrameTime();

    /// @brief Получает как текущее среднее количество кадров в секунду (fps), так и время кадра в миллисекундах.
    /// @param OutFPS Ссылка на переменную для хранения текущего среднего количества кадров в секунду (fps).
    /// @param OutFrameMs Ссылка на переменную для хранения текущего среднего времени кадра в миллисекундах.
    MAPI void Frame(f64& OutFPS, f64& OutFrameMs);

    MAPI void BeginFunction(const char* FunctionName);
    MAPI void EndFunction(const char* FunctionName);
    MAPI f64 GetFunctionExecutionTime(const char* FunctionName);
} // namespace Metrics
