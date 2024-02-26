#pragma once

template<typename T>
struct Vector4D
{
#if defined(KUSE_SIMD)
    // ������������ ��� �������� SIMD.
    alignas(16) __m128 data;
#endif

	alignas(16) T elements[4];

    // ������ �������.
    union { T x, r, s; };

    // ������ �������
    union { T y, g, t; };

	// ������ �������
	union { T z, b, p; };

	// ��������� �������
	union { T w, a, q; };

	//������������
	Vector4D() = default;
	Vector4D(float a, float b, float c, float d);

	//���������
	float& operator [](int i);
	const float& operator [](int i) const;
	Vector4D& operator *=(float s);
	Vector4D& operator /=(float s);
	Vector4D& operator +=(const Vector4D& v);
	Vector4D& operator -=(const Vector4D& v);
};

