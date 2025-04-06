#pragma once

#include "defines.hpp"
#include "containers/mstring.hpp"
#include "math/vector4d_fwd.h"

/// @brief Направленный свет, обычно используемый для имитации солнечного/лунного света.
struct DirectionalLight {
    /// @brief Имя направленного света.
    MString name;
    /// @brief Данные шейдера направленного света.
    struct Data {
        /// @brief Цвет света.
        FVec4 colour;
        /// @brief Направление света. Компонент w игнорируется.
        FVec4 direction;
    } data;
    /// @brief Отладочные данные, назначенные источнику света.
    void* DebugData;
    void* operator new(u64 size);
    void operator delete(void* ptr, u64 size);
};

/// @brief Точечный источник света, наиболее распространенный источник света, который исходит из заданной точки.
struct PointLight {
    /// @brief Имя источника света.
    MString name;
    /// @brief Данные шейдера для точечного источника света.
    struct Data {
        /// @brief Цвет света.
        FVec4 colour;
        /// @brief Положение света в мире. Компонент w игнорируется.
        FVec4 position;
        /// @brief Обычно 1, убедитесь, что знаменатель никогда не становится меньше 1
        f32 ConstantF;
        /// @brief Уменьшает интенсивность света линейно
        f32 linear;
        /// @brief Заставляет свет падать медленнее на больших расстояниях.
        f32 quadratic;
        /// @brief Дополнительное заполнение используется для выравнивания памяти. Игнорируется.
        f32 padding;
    } data;
    /// @brief Отладочные данные, назначенные источнику света.
    void* DebugData;
};