#pragma once

#include "defines.hpp"

#include "mmath.hpp"

template<typename T> class Vector4D;

template<typename T>
class Vector3D
{
public:
    union {
		// Первый элемент.
		union {T x, r;};
		// Второй элемент.
		union {T y, g;};
		// Третий элемент.
		union {T z, b;};

		T elements[3];
	};

	Vector3D() = default;
	Vector3D(T x, T y, T z);
	Vector3D(const Vector2D<T>& v);
	Vector3D(const Vector4D<T>& v);

	/// @brief нулевой вектор
    /// @return (0, 0, 0)
    static Vector3D Zero();
    /// @brief единичный вектор
    /// @return (1, 1, 1)
    static Vector3D One();
    /// @brief Вектор указывающий вверх
    /// @return (0, 1, 0)
    static Vector3D Up();
    /// @brief Вектор указывающий вниз
    /// @return (0, -1, 0)
    static Vector3D Down();
    /// @brief Вектор указывающий налево
    /// @return (-1, 0, 0)
    static Vector3D Left();
    /// @brief Вектор указывающий направо
    /// @return (1, 0, 0)
    static Vector3D Right();
	/// @brief Вектор указывающий вперед
	/// @return (0, 0, -1)
	static Vector3D Forward();
	/// @brief Вектор указывающий назад
	/// @return (0, 0, 1)
	static Vector3D Backward();

	T& operator [] (int i);
	const T& operator [](int i) const;
	Vector3D& operator +=(const Vector3D& v);
	Vector3D& operator +=(const T s);
	Vector3D& operator -=(const Vector3D& v);
	Vector3D& operator -=(const T s);
	Vector3D& operator *=(const Vector3D& v);
	Vector3D& operator *=(const T s);
	Vector3D& operator /=(const Vector3D& v);
	Vector3D& operator /=(const T s);
	Vector3D<f32>& operator-();

	Vector3D& Normalize();
};

template<typename T>
MINLINE Vector3D<T> operator +(const Vector3D<T>& a, const Vector3D<T>& b)
{
	return Vector3D<T>(a.x + b.x, a.y + b.y, a.z + b.z);
}

template<typename T>
MINLINE Vector3D<T> operator -(const Vector3D<T>& a, const Vector3D<T>& b)
{
	return Vector3D<T>(a.x - b.x, a.y - b.y, a.z - b.z);
}

template<typename T>
MINLINE Vector3D<T> operator *(const Vector3D<T>& a, const Vector3D<T>& b)
{
	return Vector3D<T>(a.x * b.x, a.y * b.y, a.z * b.z);
}

/// @brief Умножение на скаляр
/// @tparam T тип компонентов вектора и скаляра
/// @param v вектор
/// @param s скаляр
/// @return Копию вектора кмноженного на скаляр
template<typename T>
MINLINE Vector3D<T> operator *(const Vector3D<T>& v, T s)
{
	return Vector3D<T>(v.x * s, v.y * s, v.z * s);
}

template<typename T>
MINLINE Vector3D<T> operator /(const Vector3D<T>& a, const Vector3D<T>& b)
{
	return Vector3D<T>(a.x / b.x, a.y / b.y, a.z / b.z);
}

/// @brief Деление на скаляр
/// @tparam T тип компонентов вектора
/// @param v вектор
/// @param s скаляр
/// @return Копию вектора раделенного на скаляр
template<typename T>
MINLINE Vector3D<T> operator /(const Vector3D<T>& v, T s)
{
	return Vector3D<T>(v.x / s, v.y / s, v.z / s);
}

template<typename T>
MINLINE Vector3D<T> operator -(const Vector3D<T>& v)
{
	return Vector3D<T>(-v.x, -v.y, -v.z);
}

/// @brief Возвращает квадрат длины предоставленного вектора.
/// @tparam T тип коммпонентов вектора и возвращаемого квадрата длины
/// @param v вектор
/// @return длина в квадрате.
template<typename T>
MINLINE T VectorLenghtSquared(const Vector3D<T>& v)
{
	return v.x * v.x + v.y * v.y + v.z * v.z;
}

/// @brief Возвращает длину предоставленного вектора.
/// @tparam T тип коммпонентов вектора и возвращаемой длины
/// @param v вектор
/// @return длина вектора
template<typename T>
MINLINE T VectorLenght(const Vector3D<T>& v)
{
	return Math::sqrt(VectorLenghtSquared(v));
}

template<typename T>
MINLINE Vector3D<T> Normalize(const Vector3D<T>& v)
{
	return v / VectorLenght(v);
}

/// @brief Скалярное произведение предоставленных векторов. Обычно используется для расчета разницы направлений.
/// @tparam T тип коммпонентов векторов и возвращаемого значения
/// @param a первый вектор
/// @param b второй вектор
/// @return результат скалярного произведения векторов
template<typename T>
MINLINE T Dot(const Vector3D<T>& a, const Vector3D<T>& b)
{
	return a.x * b.x + a.y *b.y + a.z * b.z;
}

/// @brief Вычисляет и возвращает перекрестное произведение предоставленных векторов. Перекрестное произведение - это новый вектор, который ортогонален обоим предоставленным векторам.
/// @tparam T ти компонентов векторов
/// @param a первый вектор
/// @param b второй вектор
/// @return вектор который ортогонален вектору а и b
template<typename T>
MINLINE Vector3D<T> Cross(const Vector3D<T>& a, const Vector3D<T>& b)
{
	return Vector3D<T>(a.y * b.z - a.z * b.y,
	 				a.z * b.x - a.x * b.z,
						a.x * b.y - a.y * b.x);
}

template<typename T>
MINLINE Vector3D<T>& Distance(const Vector3D<T>& a, const Vector3D<T>& b)
{
	return VectorLenght(a - b);
}

template<typename T>
MINLINE T ScalarTripleProduct(const Vector3D<T>& a, const Vector3D<T>& b, const Vector3D<T>& c)
{
	return Dot(Cross(a, b), c);
}

template<typename T>
MINLINE Vector3D<T>& Project(const Vector3D<T>& a, const Vector3D<T>& b)
{
	return Vector3D<T>(b * (Dot(a, b) / Dot(b, b)));
}

template<typename T>
MINLINE Vector3D<T>& Reject(const Vector3D<T>& a, const Vector3D<T>& b)
{
	return Vector3D<T>(a - b * (Dot(a, b) / Dot(b, b)));
}

template <typename T>
MINLINE Vector3D<T>::Vector3D(T x, T y, T z)
{
	this->x = x;
	this->y = y;
	this->z = z;
}

template <typename T>
MINLINE Vector3D<T>::Vector3D(const Vector2D<T> &v)
:
x(v.x),
y(v.y),
z(v.z) {}

template <typename T>
MINLINE Vector3D<T>::Vector3D(const Vector4D<T> &v)
:
x(v.x),
y(v.y),
z(v.z) {}

template <typename T>
MINLINE Vector3D<T> Vector3D<T>::Zero()
{
    return Vector3D<T>(0, 0, 0);
}

template <typename T>
MINLINE Vector3D<T> Vector3D<T>::One()
{
    return Vector3D<T>(1, 1, 1);
}

template <typename T>
MINLINE Vector3D<T> Vector3D<T>::Up()
{
    return Vector3D<T>(0, 1, 0);
}

template <typename T>
MINLINE Vector3D<T> Vector3D<T>::Down()
{
    return Vector3D<T>(0, -1, 0);
}

template <typename T>
MINLINE Vector3D<T> Vector3D<T>::Left()
{
    return Vector3D<T>(-1, 0, 0);
}

template <typename T>
MINLINE Vector3D<T> Vector3D<T>::Right()
{
    return Vector3D<T>(1, 0, 0);
}

template <typename T>
MINLINE Vector3D<T> Vector3D<T>::Forward()
{
    return Vector3D<T>(0, 0, -1);
}

template <typename T>
MINLINE Vector3D<T> Vector3D<T>::Backward()
{
    return Vector3D<T>(0, 0, 1);
}

template <typename T>
MINLINE Vector3D<T> &Vector3D<T>::operator+=(const Vector3D<T> &v)
{
    x += v.x;
	y += v.y;
	z += z.y;
	return *this;
}

template <typename T>
MINLINE Vector3D<T> &Vector3D<T>::operator+=(const T s)
{
    x += s;
	y += s;
	z += s;
	return *this;
}

template <typename T>
MINLINE Vector3D<T> &Vector3D<T>::operator-=(const Vector3D<T> &v)
{
    x -= v.x;
	y -= v.y;
	z -= z.y;
	return *this;
}

template <typename T>
MINLINE Vector3D<T> &Vector3D<T>::operator-=(const T s)
{
    x -= s;
	y -= s;
	z -= s;
	return *this;
}

template <typename T>
MINLINE Vector3D<T> &Vector3D<T>::operator*=(const Vector3D<T> &v)
{
    x *= v.x;
	y *= v.y;
	z *= z.y;
	return *this;
}

template <typename T>
MINLINE Vector3D<T> &Vector3D<T>::operator*=(const T s)
{
    x *= s;
	y *= s;
	z *= s;
	return *this;
}

template <typename T>
MINLINE Vector3D<T> &Vector3D<T>::operator/=(const Vector3D<T> &v)
{
    x /= v.x;
	y /= v.y;
	z /= z.y;
	return *this;
}

template <typename T>
MINLINE Vector3D<T> &Vector3D<T>::operator/=(const T s)
{
    x /= s;
	y /= s;
	z /= s;
	return *this;
}

template <typename T>
MINLINE Vector3D<f32> &Vector3D<T>::operator-()
{
	x *= -1;
	y *= -1;
	z *= -1;

    return *this;
}

template <typename T>
MINLINE Vector3D<T> &Vector3D<T>::Normalize()
{
	*this /= VectorLenght<T>(*this);
    return *this;
}
