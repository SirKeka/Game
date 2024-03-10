#pragma once
#include "vector3d.hpp"

class Vertex3D : public Vector3D<f32>
{
public:
    //Конструкторы
    Vertex3D() = default;
    Vertex3D(f32 a, f32 b, f32 c) : Vector3D(a, b, c) {}

    //Операторы
    Vertex3D& operator =(const Vector3D<f32>& v);
};

MINLINE Vertex3D operator +(const Vertex3D& a, const Vector3D<f32>& b)
{
    return (Vertex3D(a.x + b.x, a.y + b.y, a.z + b.z));
}

MINLINE Vertex3D operator -(const Vertex3D& a, const Vector3D<f32>& b)
{
    return Vertex3D(a.x - b.x, a.y - b.y, a.z - b.z);
}

MINLINE Vector3D<f32> operator -(const Vertex3D& a, const Vertex3D& b)
{
    return Vector3D<f32>(a.x - b.x, a.y - b.y, a.z - b.z);
}