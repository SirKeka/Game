#pragma once

#include "defines.hpp"

#include "mvector2d.hpp"

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

using f32Vec2 = MVector2D<float>;
using f64Vec2 = MVector2D<double>;
using i8Vec2 = MVector2D<char>;
using i16Vec2 = MVector2D<signed short>;
using i32Vec2 = MVector2D<int>;
using i64Vec2 = MVector2D<long long>;
//using u8Vec2 = MVector2D<unsigned char>;
//using u16Vec2 = MVector2D<unsigned short>;
//using u32Vec2 = MVector2D<unsigned int>;
//using u64Vec2 = MVector2D<unsigned long long>;

namespace M::Math
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
MINLINE bool IsPowerOf2(u64 value) {
    return (value != 0) && ((value & (value - 1)) == 0);
}

MAPI i32 iRandom();
MAPI i32 RandomInRange(i32 min, i32 max);

MAPI f32 fRandom();
MAPI f32 RandomInRange(f32 min, f32 max);

} // namespace M::Math
