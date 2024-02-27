#pragma once

#include "defines.hpp"

#include "mmath.hpp"

template<typename T> struct MVector3D;

template<typename T>
struct MVector4D
{
#if defined(KUSE_SIMD)
    // Используется для операций SIMD.
    alignas(16) __m128 data;
#endif

    union { 
		// Первый элемент.
		T x;
		// Второй элемент
		T y; 
		// Третий элемент
		T z;
		// Четвертый элемент
		T w;

		alignas(16) T elements[4];
	};

	//Конструкторы
	MVector4D() {};
	MVector4D(T x, T y, T z, T w);

	/// @brief нулевой вектор
    /// @return (0, 0, 0, 0)
    static MVector4D Zero();
    /// @brief единичный вектор
    /// @return (1, 1, 1, 1)
    static MVector4D One();

	//Операторы
	//T& operator [](int i);
	//const T& operator [](int i) const;
	MVector4D& operator +=(const MVector4D& v);
	MVector4D& operator -=(const MVector4D& v);
	MVector4D& operator *=(const MVector4D& v);
	MVector4D& operator /=(const MVector4D& v);
	
	MVector4D& Normalize();
};

template <typename T>
MINLINE MVector3D<T>& Vec4toVec3(MVector4D<T>& v)
{
	return MVector3D&<T>(v.x, v.y, v.z);
}

/// @brief Возвращает новый Vector4D, используя Vector3D в качестве компонентов x, y и z и w для w.
/// @tparam T тип компонентов вектора
/// @param v трехмерный вектор
/// @return четырехмерный вектор
template <typename T>
MINLINE MVector4D<T>& Vec4fromVec3(MVector3D<T>& v, T w)
{
#if defined(MUSE_SIMD)
    MVector4D<T> OutVector;
    OutVector.data = _mm_setr_ps(x, y, z, w);
    return OutVector;
#else
    return MVector4D<T>(v.x, v.y, v.z, w);
#endif
}

template <typename T>
MINLINE MVector4D<T>::MVector4D(T x, T y, T z, T w)
{
	#if defined(MUSE_SIMD)
    data = _mm_setr_ps(x, y, z, w);
#else
	this->x = x;
	this->y = y;
	this->z = z;
	this->w = w;
#endif
}

template <typename T>
MINLINE MVector4D<T> MVector4D<T>::Zero()
{
    return MVector4D<T>(T(), T(), T(), T());
}

template <typename T>
MINLINE MVector4D<T> MVector4D<T>::One()
{
    return MVector4D<T>(1, 1, 1, 1);
}

template <typename T>
MINLINE MVector4D<T> &MVector4D<T>::operator+=(const MVector4D<T> &v)
{
    for (u64 i = 0; i < 4; i++) {
		elements[i] += v.elements[i];
	}
	return *this;
}

template <typename T>
MINLINE MVector4D<T> &MVector4D<T>::operator-=(const MVector4D<T> &v)
{
    for (u64 i = 0; i < 4; i++) {
		elements[i] -= v.elements[i];
	}
	return *this;
}

template <typename T>
MINLINE MVector4D<T> &MVector4D<T>::operator*=(const MVector4D<T> &v)
{
    for (u64 i = 0; i < 4; i++) {
		elements[i] *= v.elements[i];
	}
	return *this;
}

template <typename T>
MINLINE MVector4D<T> &MVector4D<T>::operator/=(const MVector4D<T> &v)
{
    for (u64 i = 0; i < 4; i++) {
		elements[i] /= v.elements[i];
	}
	return *this;
}

template <typename T>
MINLINE MVector4D<T> &MVector4D<T>::Normalize()
{
    this /= VectorLenght(this);
    return *this;
}

template <typename T>
MINLINE MVector4D<T> &operator +(const MVector4D<T> &a, const MVector4D<T> &b)
{
	MVector4D<T> result;
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
MINLINE T VectorLengthSquared(const MVector4D<T> v)
{
	return v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w;
}

/// @brief Возвращает длину предоставленного вектора.
/// @tparam T тип коммпонентов вектора и возвращаемой длины
/// @param v вектор
/// @return длина вектора
template<typename T>
MINLINE T VectorLength(const MVector4D<T> v)
{
	M::Math::sqrt(VectorLengthSquared(v));
}

template<typename T>
MINLINE MVector4D<T>& Normalize(const MVector4D<T>& v)
{
	return v / VectorLenght(v);
}

/// @brief Скалярное произведение предоставленных векторов. Обычно используется для расчета разницы направлений.
/// @tparam T тип коммпонентов векторов и возвращаемого значения
/// @param a первый вектор
/// @param b второй вектор
/// @return результат скалярного произведения векторов
template<typename T>
MINLINE T Dot(const MVector3D<T>& a, const MVector3D<T>& b)
{
	return a.x * b.x + a.y *b.y + a.z * b.z + a.w * b.w;
}
