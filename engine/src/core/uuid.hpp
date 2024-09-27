#pragma once
#include "defines.hpp"

struct uuid
{
    char value[37];

    constexpr uuid() : value() {}

    /// @brief Задает начальное значение генератору uuid.
    /// @param seed Начальное значение.
    static void Seed(u64 seed);

    /// @brief Генерирует универсальный уникальный идентификатор (UUID).
    /// @return вновь созданный UUID.
    static uuid Generate();
};
