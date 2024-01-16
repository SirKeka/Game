#pragma once
#include "defines.hpp"

class Clock
{
public:
    f64 StartTime;
    f64 elapsed;
public:
    Clock(/* args */);
    ~Clock();

    // Обновляет предоставленные часы. Следует вызвать непосредственно перед проверкой прошедшего времени.
    // Не влияет на не запущенные часы.
    void Update();

    // Запускает предоставленные часы. Сбрасывает прошедшее время.
    void Start();

    // Останавливает предоставленные часы. Не сбрасывает прошедшее время.
    void Stop();
};
