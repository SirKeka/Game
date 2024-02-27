#pragma once

#include "defines.hpp"

#include "mmath.hpp"

template<typename T> struct MVector4D;

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

	MVector3D& Normalize();
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

/// @brief Возвращает квадрат длины предоставленного вектора.
/// @tparam T тип коммпонентов вектора и возвращаемого квадрата длины
/// @param v вектор
/// @return длина в квадрате.
template<typename T>
MINLINE T VectorLengthSquared(const MVector3D<T> v)
{
	return v.x * v.x + v.y * v.y + v.z * v.z;
}

/// @brief Возвращает длину предоставленного вектора.
/// @tparam T тип коммпонентов вектора и возвращаемой длины
/// @param v вектор
/// @return длина вектора
template<typename T>
MINLINE T VectorLength(const MVector3D<T> v)
{
	M::Math::sqrt(VectorLengthSquared(v));
}

template<typename T>
MINLINE MVector3D<T>& Normalize(const MVector3D<T>& v)
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
	return a.x * b.x + a.y *b.y + a.z * b.z;
}

/// @brief Вычисляет и возвращает перекрестное произведение предоставленных векторов. Перекрестное произведение - это новый вектор, который ортогонален обоим предоставленным векторам.
/// @tparam T ти компонентов векторов
/// @param a первый вектор
/// @param b второй вектор
/// @return вектор который ортогонален вектору а и b
template<typename T>
MINLINE MVector3D<T>& Cross(const MVector3D<T>& a, const MVector3D<T>& b)
{
	return MVector3D<T>(a.y * b.z - a.z * b.y,
						a.z * b.x - a.x * b.z,
						a.x * b.y - a.y * b.x);
}

template<typename T>
MINLINE MVector3D<T>& Distance(const MVector3D<T>& a, const MVector3D<T>& b)
{
	return VectorLenght(a - b);
}

template<typename T>
MINLINE T ScalarTripleProduct(const MVector3D<T>& a, const MVector3D<T>& b, const MVector3D<T>& c)
{
	return Dot(Cross(a, b), c);
}

template<typename T>
MINLINE MVector3D<T>& Project(const MVector3D<T>& a, const MVector3D<T>& b)
{
	return MVector3D<T>(b * (Dot(a, b) / Dot(b, b)));
}

template<typename T>
MINLINE MVector3D<T>& Reject(const MVector3D<T>& a, const MVector3D<T>& b)
{
	return MVector3D<T>(a - b * (Dot(a, b) / Dot(b, b)));
}

template <typename T>
MINLINE MVector3D<T>::MVector3D(T x, T y, T z)
{
	this->x = x;
	this->y = y;
	this->z = z;
}

template <typename T>
MVector4D<T>& Vec4toVec3(MVector3D<T>& v, T w)
{
	return MVector4D&<T>(v.x, v.y, v.z, w);
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

template <typename T>
MINLINE MVector3D<T> &MVector3D<T>::Normalize()
{
	this /= VectorLenght(this);
    return *this;
}
