#pragma once

#include "defines.hpp"

#include "mmath.hpp"

template<typename T>
struct MVector2D
{
    union {
        // Первый элемент.
        T x; 
        // Второй элемент.
        T y;

        T elements[2];
    };

    MVector2D() {};
    MVector2D(T x, T y);
    ~MVector2D();

    /// @brief нулевой вектор
    /// @return (0, 0)
    MVector2D Zero();
    /// @brief единичный вектор
    /// @return (1, 1)
    MVector2D One();
    /// @brief Верх
    /// @return (0, 1)
    MVector2D Up();
    /// @brief Низ
    /// @return (0, -1)
    MVector2D Down();
    /// @brief Лево
    /// @return (-1, 0)
    MVector2D Left();
    /// @brief Право
    /// @return (1, 0)
    MVector2D Right();

    MVector2D& operator +=(const MVector2D& v);
    MVector2D& operator -=(const MVector2D& v);
    MVector2D& operator *=(const MVector2D& v);
    MVector2D& operator /=(const MVector2D& v);

    
    T LengthSquared();
    
    T Lenght();
    /// @brief Нормализует вектор до единичного вектора.
    /// @return Ссылку на нормализованный вектор.
    MVector2D& Normalize();
};

template<typename T>
MINLINE MVector2D<T> operator -(const MVector2D<T>& v)
{

}

template<typename T>
MINLINE MVector2D<T>::MVector2D(T x, T y)
{
    this->x = x;
    this->y = y;
}

template<typename T>
MINLINE MVector2D<T> MVector2D<T>::Zero()
{
    return Vector2D<T>(T(), T());
}

template <typename T>
MINLINE MVector2D<T> MVector2D<T>::One()
{
    return MVector2D(1, 1);
}

template <typename T>
MINLINE MVector2D<T> MVector2D<T>::Up()
{
    return MVector2D(0, 1);
}

template <typename T>
MINLINE MVector2D<T> MVector2D<T>::Down()
{
    return MVector2D(0, -1);
}

template <typename T>
MINLINE MVector2D<T> MVector2D<T>::Left()
{
    return MVector2D(-1, 0);
}

template <typename T>
MINLINE MVector2D<T> MVector2D<T>::Right()
{
    return MVector2D(1, 0);
}

template <typename T>
MINLINE MVector2D<T> &MVector2D<T>::operator+=(const MVector2D<T> &v)
{
    x += v.x;
    y += v.y;
    return *this;
}

template <typename T>
MINLINE MVector2D<T> operator+(const MVector2D<T> &a, const MVector2D<T> &b)
{
    return MVector2D<T>(a.x + b.x, a.y + b.y);
}

template <typename T>
MINLINE MVector2D<T> &MVector2D<T>::operator-=(const MVector2D<T> &v)
{
    x -= v.x;
    y -= v.y;
    return *this;
}

template <typename T>
MINLINE MVector2D<T> operator-(const MVector2D<T> &a, const MVector2D<T> &b)
{
    return MVector2D<T>(a.x - b.x, a.y - b.y);
}

template <typename T>
MINLINE MVector2D<T> &MVector2D<T>::operator*=(const MVector2D<T> &v)
{
    x *= v.x;
    y *= v.y;
    return *this;
}

template <typename T>
MINLINE MVector2D<T> operator*(const MVector2D<T> &a, const MVector2D<T> &b)
{
    return MVector2D<T>(a.x * b.x, a.y * b.y);
}

template <typename T>
MINLINE MVector2D<T> &MVector2D<T>::operator/=(const MVector2D<T> &v)
{
    x /= v.x;
    y /= v.y;
    return *this;
}

template <typename T>
MINLINE MVector2D<T> operator/(const MVector2D<T> &a, const MVector2D<T> &b)
{
    return MVector2D<T>(a.x / b.x, a.y / b.y);
}

template <typename T>
MINLINE bool operator==(const MVector2D<T> &a, const MVector2D<T> &b)
{
    if(a.x == b.x && a.y == b.y) return true;
}

template <typename T>
MINLINE bool operator!=(const MVector2D<T> &a, const MVector2D<T> &b)
{
    if(a.x != b.x || a.y != b.y) return true;
}

/// @return квадрат длины
template <typename T>
MINLINE T VectorLengthSquared(const MVector2D<T>& v)
{
    return v.x * v.x + v.y * v.y;
}

/// @return длину вектора
template <typename T>
MINLINE T VectorLenght(const MVector2D<T>& v)
{
    M::Math::sqrt(VectorLengthSquared(v));
}

template <typename T>
MINLINE MVector2D<T> &MVector2D<T>::Normalize()
{
    this /= VectorLenght(this);
}

/// @brief Возвращает нормализованную копию вектора
/// @tparam T тип компонентов
/// @param v константная ссылка на вектор
/// @return нормализованную копию предоставленного вектора
template <typename T>
MINLINE MVector2D<T>& VectorNormalize(const MVector2D<T>& v)
{
    return v / VectorLenght(v);
}

/// @brief Возвращает расстояние между вектором а и вектором b
/// @tparam T тип компонентов
/// @param a первый вектор
/// @param b второй вектор
/// @return расстояние между вектором а и вектором b
template <typename T>
MINLINE MVector2D<T>& Distance(const MVector2D<T> &a, const MVector2D<T> &b)
{
    return VectorLenght(a - b);
}