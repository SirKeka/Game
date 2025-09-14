#pragma once

#include "math.h"

template<typename T>
class Vector2D
{
public:
    union {
        // Массив x, y
        T elements[2];
        struct {
            // Первый элемент.
			union {T x, r;};
            // Второй элемент.
			union {T y, g;};
        };
    };

    constexpr Vector2D() : x(), y() {}
    constexpr Vector2D(T x, T y) : x(x), y(y) {}
    //~Vector2D();

    /// @brief нулевой вектор
    /// @return (0, 0)
    constexpr Vector2D& Zero() {
        return Vector2D<T>(T(), T());
    }
    /// @brief единичный вектор
    /// @return (1, 1)
    constexpr Vector2D& One() {
        return Vector2D(1, 1);
    }
    /// @brief Верх
    /// @return (0, 1)
    constexpr Vector2D& Up() {
        return Vector2D(0, 1);
    }
    /// @brief Низ
    /// @return (0, -1)
    constexpr Vector2D& Down() {
        return Vector2D(0, -1);
    }
    /// @brief Лево
    /// @return (-1, 0)
    constexpr Vector2D& Left() {
        return Vector2D(-1, 0);
    }
    /// @brief Право
    /// @return (1, 0)
    constexpr Vector2D& Right() {
        return Vector2D(1, 0);
    }

    Vector2D& operator +=(const Vector2D& v);
    Vector2D& operator -=(const Vector2D& v);
    Vector2D& operator *=(const Vector2D& v);
    Vector2D& operator /=(const Vector2D& v);
    bool operator==(const Vector2D& v) const {
        if (Math::abs(x - v.x) > M_FLOAT_EPSILON) {
			return false;
		}
        if (Math::abs(y - v.y) > M_FLOAT_EPSILON) {
            return false;
        }
		return true;
    }

    
    T LengthSquared();
    
    T Lenght();
    /// @brief Нормализует вектор до единичного вектора.
    /// @return Ссылку на нормализованный вектор.
    Vector2D& Normalize();
};

template<typename T>
MINLINE Vector2D<T> operator -(const Vector2D<T>& v)
{

}

template <typename T>
MINLINE Vector2D<T> &Vector2D<T>::operator+=(const Vector2D<T> &v)
{
    x += v.x;
    y += v.y;
    return *this;
}

template <typename T>
MINLINE Vector2D<T> operator+(const Vector2D<T> &a, const Vector2D<T> &b)
{
    return Vector2D<T>(a.x + b.x, a.y + b.y);
}

template <typename T>
MINLINE Vector2D<T> &Vector2D<T>::operator-=(const Vector2D<T> &v)
{
    x -= v.x;
    y -= v.y;
    return *this;
}

template <typename T>
MINLINE Vector2D<T> operator-(const Vector2D<T> &a, const Vector2D<T> &b)
{
    return Vector2D<T>(a.x - b.x, a.y - b.y);
}

template <typename T>
MINLINE Vector2D<T> &Vector2D<T>::operator*=(const Vector2D<T> &v)
{
    x *= v.x;
    y *= v.y;
    return *this;
}

template <typename T>
MINLINE Vector2D<T> operator*(const Vector2D<T> &a, const Vector2D<T> &b)
{
    return Vector2D<T>(a.x * b.x, a.y * b.y);
}

template <typename T>
MINLINE Vector2D<T> &Vector2D<T>::operator/=(const Vector2D<T> &v)
{
    x /= v.x;
    y /= v.y;
    return *this;
}

template <typename T>
MINLINE Vector2D<T> operator/(const Vector2D<T> &a, const Vector2D<T> &b)
{
    return Vector2D<T>(a.x / b.x, a.y / b.y);
}

template <typename T>
MINLINE bool operator==(const Vector2D<T> &a, const Vector2D<T> &b)
{
    if(a.x == b.x && a.y == b.y) return true;
}

template <typename T>
MINLINE bool operator!=(const Vector2D<T> &a, const Vector2D<T> &b)
{
    if(a.x != b.x || a.y != b.y) return true;
}

/// @brief Возвращает квадрат длины предоставленного вектора.
/// @tparam T тип коммпонентов вектора и возвращаемой длины
/// @param v вектор
/// @return длина в квадрате.
template <typename T>
MINLINE T VectorLengthSquared(const Vector2D<T>& v)
{
    return v.x * v.x + v.y * v.y;
}

/// @return длину вектора
template <typename T>
MINLINE T VectorLenght(const Vector2D<T>& v)
{
    Math::sqrt(VectorLengthSquared(v));
}

template <typename T>
MINLINE Vector2D<T> &Vector2D<T>::Normalize()
{
    this /= VectorLenght(this);
    return *this;
}

/// @brief Возвращает нормализованную копию вектора
/// @tparam T тип компонентов
/// @param v константная ссылка на вектор
/// @return нормализованную копию предоставленного вектора
template <typename T>
MINLINE Vector2D<T>& VectorNormalize(const Vector2D<T>& v)
{
    return v / VectorLenght(v);
}

/// @brief Сравнивает все элементы вектора a и вектора b и гарантирует, что разница меньше допуска.
/// @param a первый вектор.
/// @param b второй вектор
/// @param tolerance Допустимая разница. Обычно M_FLOAT_EPSILON или аналогичная.
/// @return true, если в пределах допустимого; в противном случае false
template <typename T>
MINLINE bool Compare(const Vector2D<T> &a, const Vector2D<T> &b, T tolerance) {
    if (Math::abs(a.x - b.x) > tolerance) {
        return false;
    }

    if (Math::abs(a.y - b.y) > tolerance) {
        return false;
    }

    return true;
}

/// @brief Возвращает расстояние между вектором а и вектором b
/// @tparam T тип компонентов
/// @param a первый вектор
/// @param b второй вектор
/// @return расстояние между вектором а и вектором b
template <typename T>
MINLINE Vector2D<T>& Distance(const Vector2D<T> &a, const Vector2D<T> &b)
{
    return VectorLenght(a - b);
}