#pragma once
#include "vector2d_fwd.hpp"
#include "vector3d_fwd.hpp"
#include "vector4d_fwd.hpp"
#include "core/mmemory.hpp"

struct Vertex2D
{
    FVec2 position; // Позиция вершины
    FVec2 texcoord; // Текстурные коорднаты
};

/// @brief Структура вершины геометрии/сетки(меша) содержит:
/// FVec3 position - Позиция вершины
/// FVec3 normal   - Нормаль вершины
/// FVec2 texcoord - Текстурные коорднаты
/// FVec4 colour   - Цвет вершины
/// FVec4 tangent  - Касательная вершины.
struct Vertex3D
{
    FVec3 position; // Позиция вершины
    FVec3 normal;   // Нормаль вершины
    FVec2 texcoord; // Текстурные коорднаты
    FVec4 colour;   // Цвет вершины
    FVec4 tangent;  // Касательная вершины.

    const bool operator==(const Vertex3D& v3d) {
        if (position == v3d.position && normal == v3d.normal && texcoord == v3d.texcoord && colour == v3d.colour && tangent == v3d.tangent) {
            return true;
        }
        return false;
    }

    constexpr Vertex3D() : position(), normal(), texcoord(), colour(), tangent() {}
    constexpr Vertex3D(FVec3 position, FVec3 normal, FVec2 texcoord) 
    : position(position), normal(normal), texcoord(texcoord), colour(), tangent() {}
    constexpr Vertex3D(f32 PositionX, f32 positionY, f32 TexcoordX, f32 TexcoordY) 
    : position(PositionX, positionY), normal(), texcoord(TexcoordX, TexcoordY), colour(), tangent() {}
    constexpr Vertex3D(f32 PositionX, f32 positionY, f32 PositionZ, f32 NormalX, f32 NormalY, f32 NormalZ, f32 TexcoordX, f32 TexcoordY)
    : position(PositionX, positionY, PositionZ), normal(NormalX, NormalY, NormalZ), texcoord(TexcoordX, TexcoordY), colour(), tangent() {}
    //void* operator new(u64 size) { return MMemory::Allocate(size, Memory::Array); }
    //void operator delete(void* ptr, u64 size) { MMemory::Free(ptr, size, Memory::Array); }
    void* operator new[](u64 size) { 
        return MMemory::Allocate(size, Memory::Array); 
        }
    void operator delete[](void* ptr, u64 size) { 
        MMemory::Free(ptr, size, Memory::Array); 
        }
};

