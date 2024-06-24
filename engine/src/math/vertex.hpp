#pragma once
#include "vector2d.hpp"
#include "vector3d.hpp"

struct Vertex2D
{
    Vector2D<f32> position;
    Vector2D<f32> texcoord;
};

struct Vertex3D
{
    Vector3D<f32> position; // Позиция вершины
    Vector3D<f32> normal;   // Нормаль вершины
    Vector2D<f32> texcoord;
};

