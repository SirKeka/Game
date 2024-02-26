#pragma once

#include "defines.hpp"

#define M_PI 3.14159265358979323846f
#define M_PI_2 2.0f * M_PI
#define M_HALF_PI 0.5f * M_PI
#define M_QUARTER_PI 0.25f * M_PI
#define M_ONE_OVER_PI 1.0f / M_PI
#define M_ONE_OVER_TWO_PI 1.0f / M_PI_2
#define M_SQRT_TWO 1.41421356237309504880f
#define M_SQRT_THREE 1.73205080756887729352f
#define M_SQRT_ONE_OVER_TWO 0.70710678118654752440f
#define M_SQRT_ONE_OVER_THREE 0.57735026918962576450f
#define M_DEG2RAD_MULTIPLIER M_PI / 180.0f
#define M_RAD2DEG_MULTIPLIER 180.0f / M_PI

// Множитель для преобразования секунд в миллисекунды.
#define M_SEC_TO_MS_MULTIPLIER 1000.0f

// Множитель для преобразования миллисекунд в секунды.
#define M_MS_TO_SEC_MULTIPLIER 0.001f

// Огромное число, которое должно быть больше любого используемого допустимого числа.
#define M_INFINITY 1e30f

// Наименьшее положительное число, где 1.0 + FLOAT_EPSILON != 0
#define M_FLOAT_EPSILON 1.192092896e-07f

// ------------------------------------------
// Общие математические функции
// ------------------------------------------
MAPI f32 msin(f32 x);
MAPI f32 mcos(f32 x);
MAPI f32 mtan(f32 x);
MAPI f32 macos(f32 x);
MAPI f32 msqrt(f32 x);
MAPI f32 mabs(f32 x);

/**
 * Indicates if the value is a power of 2. 0 is considered _not_ a power of 2.
 * @param value The value to be interpreted.
 * @returns True if a power of 2, otherwise false.
 */
MINLINE bool IsPowerOf2(u64 value) {
    return (value != 0) && ((value & (value - 1)) == 0);
}

MAPI i32 mRandom();
MAPI i32 mRandomInRange(i32 min, i32 max);

MAPI f32 fmRandom();
MAPI f32 fmRandomInRange(f32 min, f32 max);