#pragma once

#include "matrix3d.h"
#include "vector3d_fwd.h"

class MAPI Quaternion
{
public:
#if defined(MUSE_SIMD)
    // Используется для операций SIMD.
    alignas(16) __m128 data; // или __m256
#endif

    union {
        // Массив x, y, z, w
        /*alignas(16)*/ f32 elements[4];
        struct {
            // Первый элемент.
			union {f32 x, r;};
            // Второй элемент.
			union {f32 y, g;};
			// Третий элемент.
			union {f32 z, b;};
			// Четвертый элемент.
			union {f32 w, a;};
        };
    };

	constexpr Quaternion() : x(), y(), z(), w() {}
	constexpr Quaternion(f32 x, f32 y, f32 z, f32 w) : x(x), y(y), z(z), w(w) {}
	constexpr Quaternion(const FVec3& v, f32 s) : x(v.x), y(v.y), z(v.z), w(s) {}
	/// @brief 
	/// @param axis 
	/// @param angle 
	/// @param normalize 
	constexpr Quaternion(const FVec3& axis, f32 angle, bool normalize) : x(), y(), z(), w() {
		const f32 HalfAngle = 0.5f * angle;
    	f32 s = Math::sin(HalfAngle);
    	f32 c = Math::cos(HalfAngle);

		x = s * axis.x;
		y = s * axis.y;
		z = s * axis.z;
		w = c;

    	if (normalize) this->Normalize();
	}
	//constexpr Quaternion(const Quaternion& q) : x(q.x), y(q.y), z(q.z), w(q.w) {}
	/*constexpr Quaternion(Quaternion&& q) : x(q.x), y(q.y), z(q.z), w(q.w) {
		q.x = 0;
		q.y = 0;
		q.z = 0;
		q.w = 0;
	}*/

	Quaternion& operator-();
	Quaternion& operator*=(const Quaternion& q);
	//Quaternion& operator=(const Quaternion& q);

	Vector3D<f32>& GetVectorPart(void);
	const Vector3D<f32>& GetVectorPart(void) const;

	Matrix3D GetRotationMatrix(void);
	void SetRotationMatrix(const Matrix3D& m);
	
	// Функция нормализует текущий кватернион
	Quaternion& Normalize();

	static MINLINE Quaternion Identity() {
		return Quaternion(0.f, 0.0f, 0.0f, 1.0f);
	}
	
};

Quaternion operator *(const Quaternion& q1, const Quaternion& q2);

/// @brief Функция вычисляющая норму кватерниона
/// @param q кватернион норму которого нужно найти
/// @return значение нормы кватерниона
MINLINE f32 Normal(const Quaternion& q)
{
    return Math::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
}

/// @brief Функция нормализует принимаемый кватернилн
/// @param q кватернион который нужно нормалищовать
/// @return копию нормализованного кватерниона
MINLINE Quaternion Normalize(const Quaternion& q)
{
	f32 n = Normal(q);
    return Quaternion(q.x / n, q.y / n, q.z / n, q.w / n);
}

/// @brief Функция вычисляет сопряженное число кватерниона
/// @param q кватернион сопряженное число которого нужно найти
/// @return Quaterrnion(-x, -y, -z, w)
MINLINE Quaternion Conjugate(const Quaternion& q)
{
	return Quaternion(-q.x, -q.y, -q.z, q.w);
}

MINLINE Quaternion Inverse(const Quaternion& q)
{
	return Normalize(Conjugate(q));
}

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

/// @brief Эта функция вращает вектор v, используя кватернион q, вычисляя сэндвич-произведение. Предполагается, что q — единичный кватернион.
/// @param v 
/// @param q
/// @return 
MINLINE FVec3 Rotate(const FVec3& v, const Quaternion& q) 
{
	const FVec3& b = q.GetVectorPart();
    f32 b2 = b.x * b.x + b.y * b.y + b.z * b.z;

	return (v * (q.w * q.w - b2) + b * (Dot(v, b) * 2.F) + Cross(b, v) * (q.w * 2.F));
}