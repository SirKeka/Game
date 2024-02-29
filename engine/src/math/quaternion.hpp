#pragma once

#include "defines.hpp"
#include "mmatrix3d.hpp"

template<typename T> struct Vector3D;

class Quaternion
{
public:
#if defined(MUSE_SIMD)
    // Используется для операций SIMD.
    alignas(16) __m128 data; // или __m256
#endif

    union { 
		// Первый элемент.
		union {f32 x, r;};
		// Второй элемент.
		union {f32 y, g;};
		// Третий элемент.
		union {f32 z, b;};
		// Четвертый элемент.
		union {f32 w, a;};

		alignas(16) f32 elements[4];
	};

	Quaternion() = default;
	Quaternion(f32 a, f32 b, f32 c, f32 s);
	Quaternion(const Vector3D<f32>& v, f32 s);
	/// @brief 
	/// @param axis 
	/// @param angle 
	/// @param normalize 
	Quaternion(const Vector3D<f32>& axis, f32 angle, bool normalize = false);
	Quaternion(const Quaternion& q);
	Quaternion(Quaternion&& q);

	Quaternion& operator-();
	Quaternion& operator=(const Quaternion& q);

	Vector3D<f32>& GetVectorPart(void);
	const Vector3D<f32>& GetVectorPart(void) const;

	Matrix3D GetRotationMatrix(void);
	void SetRotationMatrix(const Matrix3D& m);
	
	// Функция нормализует текущий кватернион
	MINLINE const Quaternion& Normalize();

	static MINLINE Quaternion Identity();
	
};

Quaternion operator *(const Quaternion& q1, const Quaternion& q2);

/// @brief Функция вычисляющая норму кватерниона
/// @param q кватернион норму которого нужно найти
/// @return значение нормы кватерниона
MINLINE f32 Normal(const Quaternion& q);
/// @brief Функция нормализует принимаемый кватернилн
/// @param q кватернион который нужно нормалищовать
/// @return копию нормализованного кватерниона
MINLINE Quaternion Normalize(const Quaternion& q);
/// @brief Функция вычисляет сопряженное число кватерниона
/// @param q кватернион сопряженное число которого нужно найти
/// @return Quaterrnion(-x, -y, -z, w)
MINLINE Quaternion Conjugate(const Quaternion& q);
MINLINE Quaternion Inverse(const Quaternion& q);
/// @brief 
/// @param q1 
/// @param q2 
/// @return 
MINLINE f32 Dot(const Quaternion& q1, const Quaternion& q2);
/// @brief Cферическая линейная интерполяция предоставляет возможность плавной интерполяции точки между двумя ориентациями
/// @param q1 первая ориентация
/// @param q2 вторая ориентация
/// @param percentage 
/// @return 
MINLINE Quaternion SLERP(const Quaternion& q1, const Quaternion& q2, f32 percentage);
