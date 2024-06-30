#pragma once

#include "defines.hpp"

constexpr f32 M_PI = 3.14159265358979323846f;
constexpr f32 M_PI_2 = 2.0f * M_PI;
constexpr f32 M_HALF_PI = 0.5f * M_PI;
constexpr f32 M_QUARTER_PI = 0.25f * M_PI;
constexpr f32 M_ONE_OVER_PI = 1.0f / M_PI;
constexpr f32 M_ONE_OVER_TWO_PI = 1.0f / M_PI_2;
constexpr f32 M_SQRT_TWO = 1.41421356237309504880f;
constexpr f32 M_SQRT_THREE = 1.73205080756887729352f;
constexpr f32 M_SQRT_ONE_OVER_TWO = 0.70710678118654752440f;
constexpr f32 M_SQRT_ONE_OVER_THREE = 0.57735026918962576450f;
constexpr f32 M_DEG2RAD_MULTIPLIER = M_PI / 180.0f;
constexpr f32 M_RAD2DEG_MULTIPLIER = 180.0f / M_PI;

// Множитель для преобразования секунд в миллисекунды.
constexpr f32 M_SEC_TO_MS_MULTIPLIER = 1000.0f;

// Множитель для преобразования миллисекунд в секунды.
constexpr f32 M_MS_TO_SEC_MULTIPLIER = 0.001f;

// Огромное число, которое должно быть больше любого используемого допустимого числа.
constexpr f32 M_INFINITY = 1e30f;

// Наименьшее положительное число, где 1.0 + FLOAT_EPSILON != 0
constexpr f32 M_FLOAT_EPSILON = 1.192092896e-07f;

namespace Math
{
// ------------------------------------------
// Общие математические функции
// ------------------------------------------

MAPI f32 sin(f32 x);
MAPI f32 cos(f32 x);
MAPI f32 tan(f32 x);
MAPI f32 acos(f32 x);
MAPI f32 sqrt(f32 x);
MAPI f32 abs(f32 x);

/**
 * Указывает, является ли значение степенью 2. 0 считается _не_ степенью 2.
 * @param value Значение, подлежащее интерпретации.
 * @returns Истинно, если степень равна 2, в противном случае ложно.
 */
MINLINE bool IsPowerOf2(u64 value) 
{
    return (value != 0) && ((value & (value - 1)) == 0);
}

MAPI i32 iRandom();
MAPI i32 RandomInRange(i32 min, i32 max);

MAPI f32 fRandom();
MAPI f32 RandomInRange(f32 min, f32 max);

/// @brief Преобразует указанные градусы в радианы.
/// @param degrees градусы, подлежащие преобразованию.
/// @return величина в радианах.
MINLINE f32 DegToRad(f32 degrees) 
{
    return degrees * M_DEG2RAD_MULTIPLIER;
}

/// @brief Преобразует указанные радианы в градусы.
/// @param radians радианы, подлежащие преобразованию.
/// @return величина в градусах.
MINLINE f32 RadToDeg(f32 radians) 
{
    return radians * M_RAD2DEG_MULTIPLIER;
}

} // namespace M::Math
