#pragma once

template<typename T>
struct Vector4D
{
#if defined(KUSE_SIMD)
    // Используется для операций SIMD.
    alignas(16) __m128 data;
#endif

	alignas(16) T elements[4];

    // Первый элемент.
    union { T x, r, s; };

    // Второй элемент
    union { T y, g, t; };

	// Третий элемент
	union { T z, b, p; };

	// Четвертый элемент
	union { T w, a, q; };

	//Конструкторы
	Vector4D() = default;
	Vector4D(float a, float b, float c, float d);

	//Операторы
	float& operator [](int i);
	const float& operator [](int i) const;
	Vector4D& operator *=(float s);
	Vector4D& operator /=(float s);
	Vector4D& operator +=(const Vector4D& v);
	Vector4D& operator -=(const Vector4D& v);
};

