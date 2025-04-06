#pragma once

#include "defines.hpp"

#include "math.h"
#include "vector3d.h"

template<typename T>
struct Vector4D
{
#if defined(MUSE_SIMD)
    // Используется для операций SIMD.
    alignas(16) __m128 data; // или __m256
#endif

	union {
        // Массив x, y, z, w
        T elements[4];
        struct {
            // Первый элемент.
			union {T x, r;};
            // Второй элемент.
			union {T y, g;};
			// Третий элемент.
			union {T z, b;};
			// Четвертый элемент.
			union {T w, a;};
        };
    };

	constexpr Vector4D() noexcept : x(), y(), z(), w() {};
	constexpr explicit Vector4D(T x, T y, T z, T w) noexcept : x(x), y(y), z(z), w(w) {
#if defined(MUSE_SIMD)
    	data = _mm_setr_ps(x, y, z, w);
#endif
	}

	constexpr explicit Vector4D(const Vector2D<T>& v, f32 z = 0, f32 w = 0) noexcept : x(v.x), y(v.y), z(z), w(w) {}
	constexpr explicit Vector4D(const Vector3D<T>& v, f32 w = 0) noexcept : x(v.x), y(v.y), z(v.z), w(w) {
#if defined(MUSE_SIMD)
    	Vector4D<T> OutVector;
    	OutVector.data = _mm_setr_ps(x, y, z, w);
    	return OutVector;
#endif
	}

	constexpr explicit Vector4D(const Vector4D& v) noexcept : x(v.x), y(v.y), z(v.z), w(v.w) {}

	Vector4D& Set(T x, T y, T z, T w) {
		this->x = x;
		this->y = y;
		this->z = z;
		this->w = w;
		return *this;
	}

	/// @brief Нулевой вектор
    /// @return (0, 0, 0, 0)
    static MINLINE Vector4D Zero() {
	    return Vector4D(T(), T(), T(), T());
	}
    /// @brief Единичный вектор
    /// @return (1, 1, 1, 1)
    static constexpr Vector4D One() {
	    return Vector4D(1, 1, 1, 1);
	}

	//T& operator [](int i);
	//const T& operator [](int i) const;
	Vector4D& operator +=(const Vector4D& v) {
		for (u64 i = 0; i < 4; i++) {
			elements[i] += v.elements[i];
		}
		return *this;
	}

	Vector4D& operator -=(const Vector4D& v) {
		for (u64 i = 0; i < 4; i++) {
			elements[i] -= v.elements[i];
		}
		return *this;
	}

	Vector4D& operator *=(const Vector4D& v) {
		for (u64 i = 0; i < 4; i++) {
			elements[i] *= v.elements[i];
		}
		return *this;
	}

	Vector4D& operator /=(const Vector4D& v) {
		for (u64 i = 0; i < 4; i++) {
			elements[i] /= v.elements[i];
		}
		return *this;
	}

	explicit operator bool() const noexcept {
		if ((x != 0) && (y != 0) && (z != 0) && (w != 0)) return true;
		return false;
	}

	bool operator==(const Vector4D& v) const {
		if (Math::abs(x - v.x) > M_FLOAT_EPSILON) {
			return false;
		}
        if (Math::abs(y - v.y) > M_FLOAT_EPSILON) {
            return false;
        }
		if (Math::abs(z - v.z) > M_FLOAT_EPSILON) {
			return false;
		}
		if (Math::abs(w - v.w) > M_FLOAT_EPSILON) {
			return false;
		}
		return true;
	}

	Vector4D& operator= (const Vector4D& v) {
		x = v.x; y = v.y; z = v.z; w = v.w;
		return *this;
	}
	
	Vector4D& Normalize() {
		this /= VectorLenght(this);
    	return *this;
	}
};

template <typename T>
MINLINE Vector4D<T> &operator +(const Vector4D<T> &a, const Vector4D<T> &b)
{
	Vector4D<T> result;
    for (u64 i = 0; i < 4; i++) {
		result = a.elements[i] + b.elements[i];
	}
	return result;
}

/// @brief Возвращает квадрат длины предоставленного вектора.
/// @tparam T тип коммпонентов вектора и возвращаемого квадрата длины
/// @param v вектор
/// @return длина в квадрате.
template<typename T>
MINLINE T VectorLengthSquared(const Vector4D<T>& v)
{
	return v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
}

/// @brief Возвращает длину предоставленного вектора.
/// @tparam T тип коммпонентов вектора и возвращаемой длины
/// @param v вектор
/// @return длина вектора
template<typename T>
MINLINE T VectorLength(const Vector4D<T>& v)
{
	Math::sqrt(VectorLengthSquared(v));
}

template<typename T>
MINLINE Vector4D<T>& Normalize(const Vector4D<T>& v)
{
	return v / VectorLenght(v);
}

/// @brief Скалярное произведение предоставленных векторов. Обычно используется для расчета разницы направлений.
/// @tparam T тип коммпонентов векторов и возвращаемого значения
/// @param a первый вектор
/// @param b второй вектор
/// @return результат скалярного произведения векторов
template<typename T>
MINLINE T Dot(const Vector4D<T>& a, const Vector4D<T>& b)
{
	return a.x * b.x + a.y *b.y + a.z * b.z + a.w * b.w;
}
