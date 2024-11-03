#pragma once

#include "defines.hpp"

constexpr f32 M_PI = 3.14159265358979323846F;
constexpr f32 M_PI_2 = 2.0F * M_PI;
constexpr f32 M_HALF_PI = 0.5F * M_PI;
constexpr f32 M_QUARTER_PI = 0.25F * M_PI;
constexpr f32 M_ONE_OVER_PI = 1.F / M_PI;
constexpr f32 M_ONE_OVER_TWO_PI = 1.F / M_PI_2;
constexpr f32 M_SQRT_TWO = 1.41421356237309504880F;
constexpr f32 M_SQRT_THREE = 1.73205080756887729352F;
constexpr f32 M_SQRT_ONE_OVER_TWO = 0.70710678118654752440F;
constexpr f32 M_SQRT_ONE_OVER_THREE = 0.57735026918962576450F;
constexpr f32 M_DEG2RAD_MULTIPLIER = M_PI / 180.F;
constexpr f32 M_RAD2DEG_MULTIPLIER = 180.F / M_PI;

/** @brief Множитель для преобразования секунд в микросекунды. */
constexpr f32 M_SEC_TO_US_MULTIPLIER = 1000.F * 1000.F;

// Множитель для преобразования секунд в миллисекунды.
constexpr f32 M_SEC_TO_MS_MULTIPLIER = 1000.F;

// Множитель для преобразования миллисекунд в секунды.
constexpr f32 M_MS_TO_SEC_MULTIPLIER = 0.001F;

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

/// @brief Преобразует значение из «старого» диапазона в «новый» диапазон.
/// @param value Значение для преобразования.
/// @param OldMin Минимальное значение из старого диапазона.
/// @param OldMax Максимальное значение из старого диапазона.
/// @param NewMin Минимальное значение из нового диапазона.
/// @param NewMax Максимальное значение из нового диапазона.
/// @return Преобразованное значение.
MINLINE f32 RangeConvertF32(f32 value, f32 OldMin, f32 OldMax, f32 NewMin, f32 NewMax) {
    return (((value - OldMin) * (NewMax - NewMin)) / (OldMax - OldMin)) + NewMin;
}

/// @brief Преобразует значения rgb int [0-255] в одно 32-битное целое число.
/// @param r Значение красного [0-255].
/// @param g Значение зеленого [0-255].
/// @param b Значение синего [0-255].
/// @param OutU32 Указатель для хранения полученного целого числа.
MINLINE void RgbuToU32(u32 r, u32 g, u32 b, u32& OutU32) {
    OutU32 = (((r & 0x0FF) << 16) | ((g & 0x0FF) << 8) | (b & 0x0FF));
}

/// @brief Преобразует заданное 32-битное целое число в значения RGB [0-255].
/// @param rgbu Целое число, содержащее значение RGB.
/// @param OutR Указатель для хранения значения красного.
/// @param OutG Указатель для хранения значения зеленого.
/// @param OutB Указатель для хранения значения синего.
MINLINE void u32ToRGB(u32 rgbu, u32& OutR, u32& OutG, u32& OutB) {
    OutR = (rgbu >> 16) & 0x0FF;
    OutG = (rgbu >> 8) & 0x0FF;
    OutB = (rgbu)&0x0FF;
}

} // namespace M::Math
