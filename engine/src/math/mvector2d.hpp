#pragma once

#include "defines.hpp"

#include "mmath.hpp"

template<typename T>
class Vector2D
{
public:
    union {
        // Первый элемент.
        T x; 
        // Второй элемент.
        T y;

        T elements[2];
    };

    Vector2D() {};
    Vector2D(T x, T y);
    ~Vector2D();

    /// @brief нулевой вектор
    /// @return (0, 0)
    Vector2D Zero();
    /// @brief единичный вектор
    /// @return (1, 1)
    Vector2D One();
    /// @brief Верх
    /// @return (0, 1)
    Vector2D Up();
    /// @brief Низ
    /// @return (0, -1)
    Vector2D Down();
    /// @brief Лево
    /// @return (-1, 0)
    Vector2D Left();
    /// @brief Право
    /// @return (1, 0)
    Vector2D Right();

    Vector2D& operator +=(const Vector2D& v);
    Vector2D& operator -=(const Vector2D& v);
    Vector2D& operator *=(const Vector2D& v);
    Vector2D& operator /=(const Vector2D& v);

    
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

template<typename T>
MINLINE Vector2D<T>::Vector2D(T x, T y)
{
    this->x = x;
    this->y = y;
}

template<typename T>
MINLINE Vector2D<T> Vector2D<T>::Zero()
{
    return Vector2D<T>(T(), T());
}

template <typename T>
MINLINE Vector2D<T> Vector2D<T>::One()
{
    return Vector2D(1, 1);
}

template <typename T>
MINLINE Vector2D<T> Vector2D<T>::Up()
{
    return Vector2D(0, 1);
}

template <typename T>
MINLINE Vector2D<T> Vector2D<T>::Down()
{
    return Vector2D(0, -1);
}

template <typename T>
MINLINE Vector2D<T> Vector2D<T>::Left()
{
    return Vector2D(-1, 0);
}

template <typename T>
MINLINE Vector2D<T> Vector2D<T>::Right()
{
    return Vector2D(1, 0);
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
    if (M::Math::abs(a.x - b.x) > tolerance) {
        return false;
    }

    if (M::Math::abs(a.y - b.y) > tolerance) {
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