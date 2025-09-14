#pragma once

#include "defines.h"
// #include "vector3d_fwd.hpp"
// #include "matrix4d.hpp"

/// @brief Приблизительное представление числа ПИ.
constexpr f32 M_PI = 3.14159265358979323846F;
/// @brief Приблизительное представление числа ПИ, умноженного на 2.
constexpr f32 M_2PI = 2.F * M_PI;
/// @brief риблизительное представление числа ПИ, умноженного на 4.
constexpr f32 M_4PI = 4.F * M_2PI;
/// @brief Приблизительное представление числа ПИ, деленного на 2.
constexpr f32 M_HALF_PI = 0.5F * M_PI;
/// @brief Приблизительное представление числа ПИ, деленного на 4.
constexpr f32 M_QUARTER_PI = 0.25F * M_PI;
/// @brief Единица, деленная на приблизительное значение числа ПИ.
constexpr f32 M_ONE_OVER_PI = 1.F / M_PI;
/// @brief Единица, деленная на приблизительное значение числа 2ПИ.
constexpr f32 M_ONE_OVER_TWO_PI = 1.F / M_2PI;
/// @brief Приблизительное значение квадратного корня из 2.
constexpr f32 M_SQRT_TWO = 1.41421356237309504880F;
/// @brief Приблизительное значение квадратного корня из 3.
constexpr f32 M_SQRT_THREE = 1.73205080756887729352F;
/// @brief Единица, деленная на приближенное значение квадратного корня из 2.
constexpr f32 M_SQRT_ONE_OVER_TWO = 0.70710678118654752440F;
/// @brief Единица, деленная на приближенное значение квадратного корня из 3.
constexpr f32 M_SQRT_ONE_OVER_THREE = 0.57735026918962576450F;
/// @brief Множитель, используемый для преобразования градусов в радианы.
constexpr f32 M_DEG2RAD_MULTIPLIER = M_PI / 180.F;
/// @brief Множитель, используемый для преобразования радиан в градусы.
constexpr f32 M_RAD2DEG_MULTIPLIER = 180.F / M_PI;

/** @brief Множитель для преобразования секунд в микросекунды. */
constexpr f32 M_SEC_TO_US_MULTIPLIER = 1000.F * 1000.F;

// Множитель для преобразования секунд в миллисекунды.
constexpr f32 M_SEC_TO_MS_MULTIPLIER = 1000.F;

// Множитель для преобразования миллисекунд в секунды.
constexpr f32 M_MS_TO_SEC_MULTIPLIER = 0.001F;

// Огромное число, которое должно быть больше любого используемого допустимого числа.
constexpr f32 M_INFINITY = 1e30f * 1e30f;

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

    /// @brief Выполнить интерполяцию Эрмита между двумя значениями.
    /// @param edge_0 Нижний край функции Эрмита.
    /// @param edge_1 Верхний край функции Эрмита.
    /// @param x Значение для интерполяции.
    /// @return Интерполированное значение.
    MINLINE f32 Smoothstep(f32 Edge0, f32 Edge1, f32 x) {
        f32 t = MCLAMP((x - Edge0) / (Edge1 - Edge0), 0.F, 1.F);
        return t * t * (3.F - 2.F * t);
    }

    MINLINE bool FloatCompare(f32 f0, f32 f1) {
        return abs(f0 - f1) < M_FLOAT_EPSILON;
    }

    /// @brief Преобразует указанные градусы в радианы.
    /// @param degrees градусы, подлежащие преобразованию.
    /// @return величина в радианах.
    MINLINE constexpr f32 DegToRad(f32 degrees = 1.F) {
        return degrees * M_DEG2RAD_MULTIPLIER;
    }

    /// @brief Преобразует указанные радианы в градусы.
    /// @param radians радианы, подлежащие преобразованию.
    /// @return величина в градусах.
    MINLINE f32 RadToDeg(f32 radians)  {
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

	/// @brief Преобразовать v по m. ПРИМЕЧАНИЕ: эта функция предполагает, что вектор v является точкой, а не направлением, и вычисляется так, как если бы там был компонент w со значением 1.0f.
	/// @param v Вектор для преобразования.
	/// @param m Матрица для преобразования.
	/// @return Преобразованная копия v.
	// MINLINE FVec3 Transform(const FVec3& v, const Matrix4D& m) {
	// 	FVec3 out;
	// 	out.x = v.x * m.data[0 + 0] + v.y * m.data[4 + 0] + v.z * m.data[8 + 0] + 1.0f * m.data[12 + 0];
    // 	out.y = v.x * m.data[0 + 1] + v.y * m.data[4 + 1] + v.z * m.data[8 + 1] + 1.0f * m.data[12 + 1];
    // 	out.z = v.x * m.data[0 + 2] + v.y * m.data[4 + 2] + v.z * m.data[8 + 2] + 1.0f * m.data[12 + 2];
	// 	return out;
	// }

} // namespace Math
