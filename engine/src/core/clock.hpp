#pragma once
#include "defines.h"

class MAPI Clock
{
public:
    f64 StartTime;
    f64 elapsed;
public:
    constexpr Clock() : StartTime(), elapsed() {}
    ~Clock();

    // Обновляет предоставленные часы. Следует вызвать непосредственно перед проверкой прошедшего времени.
    // Не влияет на не запущенные часы.
    void Update();

    // Запускает предоставленные часы. Сбрасывает прошедшее время.
    void Start();

    // Останавливает предоставленные часы. Не сбрасывает прошедшее время.
    void Stop();

    void Zero();
};
