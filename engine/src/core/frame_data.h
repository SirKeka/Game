#pragma once

#include "defines.h"

/// @brief Данные текущего кадра на уровне движка.
struct FrameData {
    /// @brief Время в секундах с момента последнего кадра.
    f32 DeltaTime;

    /// @brief Общее время в секундах, в течение которого приложение работало.
    f64 TotalTime;

    /// @brief Количество сеток, отрисованных в последнем кадре.
    u32 DrawnMeshCount;

    /// @brief Указатель на распределитель кадров движка.
    struct LinearAllocator* FrameAllocator;

    /// @brief Данные кадра на уровне приложения. Необязательно, приложение должно знать, как использовать это, если необходимо.
    void* ApplicationFrameData;
};