#pragma once

// #include "math.h"
#include "vector3d.h"

template<typename T>
struct Vector4D
{
	union {
		#if defined(MUSE_SIMD)
			// Используется для операций SIMD.
			alignas(16) __m128 data; // или __m256
		#endif
		// Массив x, y, z, w
        T elements[4];
        struct {
            // Первый элемент.
			union {T x, r;};
            // Второй элемент.
			union {T y, g;};
			// Третий элемент.
			union {T z, b, width;};
			// Четвертый элемент.
			union {T w, a, height;};
        };
    };

	constexpr Vector4D() : x(), y(), z(), w() {};
	constexpr Vector4D(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {
#if defined(MUSE_SIMD)
    	data = _mm_setr_ps(x, y, z, w);
#endif
	}

	constexpr Vector4D(Vector2D<T> v, f32 z = 0, f32 w = 0) : x(v.x), y(v.y), z(z), w(w) {}
	constexpr Vector4D(Vector3D<T> v, f32 w = 0) : x(v.x), y(v.y), z(v.z), w(w) {
#if defined(MUSE_SIMD)
    	Vector4D<T> OutVector;
    	OutVector.data = _mm_setr_ps(x, y, z, w);
    	return OutVector;
#endif
	}

	// constexpr Vector4D(const Vector4D& v) : x(v.x), y(v.y), z(v.z), w(v.w) {}

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
	Vector4D& operator +=(Vector4D v) {
		x += v.x; y += v.y; z += v.z; w += v.w;
		return *this;
	}

	Vector4D& operator -=(Vector4D v) {
		x -= v.x; y -= v.y; z -= v.z; w -= v.w;
		return *this;
	}

	Vector4D& operator *=(Vector4D v) {
		x *= v.x; y *= v.y; z *= v.z; w *= v.w;
		return *this;
	}

	Vector4D& operator /=(Vector4D v) {
		x /= v.x; y /= v.y; z /= v.z; w /= v.w;
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

	// Vector4D& operator= (Vector4D v) {
	// 	x = v.x; y = v.y; z = v.z; w = v.w;
	// 	return *this;
	// }
	
	Vector4D& Normalize() {
		this /= VectorLenght(this);
    	return *this;
	}
};

template <typename T>
MINLINE Vector4D<T> operator +(Vector4D<T> a, Vector4D<T> b)
{
	return Vector4D<T>(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

template<typename T>
MINLINE Vector4D<T> operator +(Vector4D<T> v, T s)
{
	return Vector4D<T>(v.x + s, v.y + s, v.z + s, v.w + s);
}

template<typename T>
MINLINE Vector4D<T> operator -(Vector4D<T> a, Vector4D<T> b)
{
	return Vector4D<T>(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}

template<typename T>
MINLINE Vector4D<T> operator -(Vector4D<T> v, T s)
{
	return Vector4D<T>(v.x - s, v.y - s, v.z - s, v.w - s);
}

template<typename T>
MINLINE Vector4D<T> operator /(Vector4D<T> v, T s)
{
	return Vector4D<T>(v.x / s, v.y / s, v.z / s, v.w / s);
}

template<typename T>
MINLINE Vector4D<T> operator *(Vector4D<T> v, T s)
{
	return Vector4D<T>(v.x * s, v.y * s, v.z * s, v.w * s);
}

/// @brief Возвращает квадрат длины предоставленного вектора.
/// @tparam T тип коммпонентов вектора и возвращаемого квадрата длины
/// @param v вектор
/// @return длина в квадрате.
template<typename T>
MINLINE T VectorLengthSquared(Vector4D<T> v)
{
	return v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
}

/// @brief Возвращает длину предоставленного вектора.
/// @tparam T тип коммпонентов вектора и возвращаемой длины
/// @param v вектор
/// @return длина вектора
template<typename T>
MINLINE T VectorLenght(Vector4D<T> v)
{
	return Math::sqrt(VectorLengthSquared(v));
}

template<typename T>
MINLINE Vector4D<T> Normalize(Vector4D<T> v)
{
	return v / VectorLenght(v);
}

/// @brief Скалярное произведение предоставленных векторов. Обычно используется для расчета разницы направлений.
/// @tparam T тип коммпонентов векторов и возвращаемого значения
/// @param a первый вектор
/// @param b второй вектор
/// @return результат скалярного произведения векторов
template<typename T>
MINLINE T Dot(Vector4D<T> a, Vector4D<T> b)
{
	return a.x * b.x + a.y *b.y + a.z * b.z + a.w * b.w;
}
