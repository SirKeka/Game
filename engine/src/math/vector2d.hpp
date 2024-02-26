#pragma once

#include "defines.hpp"

template<typename T>
struct Vector2D
{
    T elements[2];

    // Первый элемент.
    union { T x, r, s, u; };

    // Второй элемент
    union { T y, g, t, v; };
    
    Vector2D(T x, T y);
    ~Vector2D();

    Vector2D Zero();
};

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
