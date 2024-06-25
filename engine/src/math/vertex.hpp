#pragma once
#include "vector2d.hpp"
#include "vector3d.hpp"
#include "core/mmemory.hpp"

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

    constexpr Vertex3D() : position(), normal(), texcoord() {}
    constexpr Vertex3D(Vector3D<f32> position, Vector3D<f32> normal, Vector2D<f32> texcoord) 
    : position(position), normal(normal), texcoord(texcoord) {}
    constexpr Vertex3D(f32 PositionX, f32 positionY, f32 NormalX, f32 NormalY) 
    : position(PositionX, positionY), normal(NormalX, NormalY), texcoord() {}
    constexpr Vertex3D(f32 PositionX, f32 positionY, f32 PositionZ, f32 NormalX, f32 NormalY, f32 NormalZ, f32 TexcoordX, f32 TexcoordY)
    : position(PositionX, positionY, PositionZ), normal(NormalX, NormalY, NormalZ), texcoord(TexcoordX, TexcoordY) {}
    void* operator new(u64 size) { return MMemory::Allocate(size, MemoryTag::Array); }
    void operator delete(void* ptr, u64 size) { MMemory::Free(ptr, size, MemoryTag::Array); }
    void* operator new[](u64 size) { return MMemory::Allocate(size, MemoryTag::Array); }
    void operator delete[](void* ptr, u64 size) { MMemory::Free(ptr, size, MemoryTag::Array); }
};

