#pragma once

#include "defines.hpp"

template<typename T>
struct MVector3D
{
    union {
		// Первый элемент.
		T x; 
		// Второй элемент.
		T y;
		// Третий элемент.
		T z;

		T elements[3];
	};

	MVector3D() = default;
	MVector3D(T x, T y, T z);

	/// @brief нулевой вектор
    /// @return (0, 0, 0)
    static MVector3D Zero();
    /// @brief единичный вектор
    /// @return (1, 1, 1)
    static MVector3D One();
    /// @brief Вектор указывающий вверх
    /// @return (0, 1, 0)
    static MVector3D Up();
    /// @brief Вектор указывающий вниз
    /// @return (0, -1, 0)
    static MVector3D Down();
    /// @brief Вектор указывающий налево
    /// @return (-1, 0, 0)
    static MVector3D Left();
    /// @brief Вектор указывающий направо
    /// @return (1, 0, 0)
    static MVector3D Right();
	/// @brief Вектор указывающий вперед
	/// @return (0, 0, -1)
	static MVector3D Forward();
	/// @brief Вектор указывающий назад
	/// @return (0, 0, 1)
	static MVector3D Backward();

	T& operator [] (int i);
	const T& operator [](int i) const;
	MVector3D& operator +=(const MVector3D& v);
	MVector3D& operator +=(const T s);
	MVector3D& operator -=(const MVector3D& v);
	MVector3D& operator -=(const T s);
	MVector3D& operator *=(const MVector3D& v);
	MVector3D& operator *=(const T s);
	MVector3D& operator /=(const MVector3D& v);
	MVector3D& operator /=(const T s);
};

template<typename T>
MINLINE MVector3D<T>& operator +(const MVector3D<T>& a, const MVector3D<T>& b)
{
	return MVector3D<T>(a.x + b.x, a.y + b.y, a.z + b.z);
}

template<typename T>
MINLINE MVector3D<T>& operator -(const MVector3D<T>& a, const MVector3D<T>& b)
{
	return MVector3D<T>(a.x - b.x, a.y - b.y, a.z - b.z);
}

template<typename T>
MINLINE MVector3D<T>& operator *(const MVector3D<T>& a, const MVector3D<T>& b)
{
	return MVector3D<T>(a.x * b.x, a.y * b.y, a.z * b.z);
}

/// @brief Умножение на скаляр
/// @tparam T тип компонентов вектора и скаляра
/// @param v вектор
/// @param s скаляр
/// @return Копию вектора кмноженного на скаляр
template<typename T>
MINLINE MVector3D<T>& operator *(const MVector3D<T>& v, T s)
{
	return MVector3D<T>(v.x * s, v.y * s, v.z * s);
}

template<typename T>
MINLINE MVector3D<T>& operator /(const MVector3D<T>& a, const MVector3D<T>& b)
{
	return MVector3D<T>(a.x / b.x, a.y / b.y, a.z / b.z);
}

/// @brief Деление на скаляр
/// @tparam T тип компонентов вектора
/// @param v вектор
/// @param s скаляр
/// @return Копию вектора раделенного на скаляр
template<typename T>
MINLINE MVector3D<T>& operator /(const MVector3D<T>& v, T s)
{
	return MVector3D<T>(v.x / s, v.y / s, v.z / s);
}

template<typename T>
MINLINE MVector3D<T>& operator -(const MVector3D<T>& v)
{
	return MVector3D<T>(-v.x, -v.y, -v.z);
}

template<typename T>
MINLINE T Magnitude(const MVector3D<T> v);

template<typename T>
MINLINE MVector3D<T>& Normalize(const MVector3D<T>& v);

template<typename T>
MINLINE T Dot(const MVector3D<T>& a, const MVector3D<T>& b);

template<typename T>
MINLINE MVector3D<T>& Cross(const MVector3D<T>& a, const MVector3D<T>& b);

template<typename T>
MINLINE T ScalarTripleProduct(const MVector3D<T>& a, const MVector3D<T>& b, const MVector3D<T>& c);

template<typename T>
MINLINE MVector3D<T>& Project(const MVector3D<T>& a, const MVector3D<T>& b);

template<typename T>
MINLINE MVector3D<T>& Reject(const MVector3D<T>& a, const MVector3D<T>& b);

template <typename T>
MINLINE MVector3D<T>::MVector3D(T x, T y, T z)
{
	this->x = x;
	this->y = y;
	this->z = z;
}

template <typename T>
MINLINE MVector3D<T> MVector3D<T>::Zero()
{
    return MVector3D<T>(0, 0, 0);
}

template <typename T>
MINLINE MVector3D<T> MVector3D<T>::One()
{
    return MVector3D<T>(1, 1, 1);
}

template <typename T>
MINLINE MVector3D<T> MVector3D<T>::Up()
{
    return MVector3D<T>(0, 1, 0);
}

template <typename T>
MINLINE MVector3D<T> MVector3D<T>::Down()
{
    return MVector3D<T>(0, -1, 0);
}

template <typename T>
MINLINE MVector3D<T> MVector3D<T>::Left()
{
    return MVector3D<T>(-1, 0, 0);
}

template <typename T>
MINLINE MVector3D<T> MVector3D<T>::Right()
{
    return MVector3D<T>(1, 0, 0);
}

template <typename T>
MINLINE MVector3D<T> MVector3D<T>::Forward()
{
    return MVector3D<T>(0, 0, -1);
}

template <typename T>
MINLINE MVector3D<T> MVector3D<T>::Backward()
{
    return MVector3D<T>(0, 0, 1);
}

template <typename T>
MINLINE MVector3D<T> &MVector3D<T>::operator+=(const MVector3D<T> &v)
{
    x += v.x;
	y += v.y;
	z += z.y;
	return *this;
}

template <typename T>
MINLINE MVector3D<T> &MVector3D<T>::operator+=(const T s)
{
    x += s;
	y += s;
	z += s;
	return *this;
}

template <typename T>
MINLINE MVector3D<T> &MVector3D<T>::operator-=(const MVector3D<T> &v)
{
    x -= v.x;
	y -= v.y;
	z -= z.y;
	return *this;
}

template <typename T>
MINLINE MVector3D<T> &MVector3D<T>::operator-=(const T s)
{
    x -= s;
	y -= s;
	z -= s;
	return *this;
}

template <typename T>
MINLINE MVector3D<T> &MVector3D<T>::operator*=(const MVector3D<T> &v)
{
    x *= v.x;
	y *= v.y;
	z *= z.y;
	return *this;
}

template <typename T>
MINLINE MVector3D<T> &MVector3D<T>::operator*=(const T s)
{
    x *= s;
	y *= s;
	z *= s;
	return *this;
}

template <typename T>
MINLINE MVector3D<T> &MVector3D<T>::operator/=(const MVector3D<T> &v)
{
    x /= v.x;
	y /= v.y;
	z /= z.y;
	return *this;
}

template <typename T>
MINLINE MVector3D<T> &MVector3D<T>::operator/=(const T s)
{
    x /= s;
	y /= s;
	z /= s;
	return *this;
}
