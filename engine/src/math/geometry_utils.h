#pragma once

#include "extents.h"
#include "containers/darray.h"

struct GeometryConfig;
struct Vertex3D;
struct TerrainVertex;
struct Plane;

/// @brief Представляет собой линию, начинающуюся в точке начала координат и продолжающуюся бесконечно в заданном направлении. Обычно используется для проверки попадания, выбора и т. д.
struct MAPI Ray
{
    FVec3 origin;
    FVec3 direction;

    constexpr Ray() : origin(), direction() {}
    constexpr Ray(const FVec3& position, const FVec3& direction) : origin(position), direction(direction) {}
    constexpr Ray(const FVec2& ScreenPosition, const Rect2D& ViewportRect, const FVec3& origin, const Matrix4D& view, const Matrix4D& projection);    
};

struct RaycastHit
{
    enum Type { OBB, Surface } type;
    u32 UniqueID;
    FVec3 position;
    f32 distance;
};

struct RaycastResult
{
    // Darray - создается только в том случае, если существует совпадение; в противном случае null.
    DArray<RaycastHit> hits;
};

namespace Math
{
    namespace Geometry
    {
        /// @brief Вычисляет нормали для заданных данных вершин и индексов. Изменяет вершины на месте.
        /// @param VertexCount количество вершин.
        /// @param Vertices массив вершин.
        /// @param IndexCount количество индексов.
        /// @param Indices массив вершин.
        void GenerateNormals(u32 VertexCount, Vertex3D* vertices, u32 IndexCount, u32* indices);

        /// @brief Вычисляет касательные для заданных данных вершины и индекса. Изменяет вершины на месте.
        /// @param VertexCount количество вершин.
        /// @param Vertices массив вершин.
        /// @param IndexCount количество индексов.
        /// @param Indices массив вершин.
        void CalculateTangents(u32 VertexCount, Vertex3D* vertices, u32 IndexCount, u32* indices);

        //void CalculateTangents(const DArray<Face>& triangleArray, DArray<Vertex3D> &vertices);

        /// @brief Удаляет дубликаты вершин, оставляя только уникальные. 
        /// Оставляет исходный массив вершин нетронутым. Выделяет новый массив в OutVertices. 
        /// Изменяет индексы на месте. Исходный массив вершин должен быть освобожден вызывающей стороной.
        /// @param geometry конфигурация геометрии из которой нужно удаляить дубликаты вершин
        void DeduplicateVertices(GeometryConfig& geometry);

        void GenerateTerrainNormals(u32 VertexCount, struct TerrainVertex *vertices, u32 IndexCount, u32 *indices);

        void GenerateTerrainTangents(u32 VertexCount, struct TerrainVertex *vertices, u32 IndexCount, u32 *indices);
    } // namespace Geometry

    MAPI Ray RaycastFromScreen(const FVec2& ScreenPosition, const Rect2D& ViewportRect, const FVec3& origin, const Matrix4D& view, const Matrix4D projection);

    MAPI bool RaycastAABB(Extents3D bbExtents, const Ray& ray, FVec3& OutPoint);

    MAPI bool RaycastOrientedExtents(Extents3D bbExtents, const Matrix4D& model, const Ray& ray, f32& OutDistance);

    MAPI bool RaycastPlane(const Ray& ray, const Plane& plane, FVec3& OutPoint, f32& OutDistance);

    MAPI bool RaycastDisc(const Ray& ray, const FVec3& center, const FVec3& normal, f32 OuterRadius, f32 InnerRadius, FVec3& OutPoint, f32& OutDistance);

    MINLINE bool PointInRect2d(const Point& point, const Rect2D& rect) {
    return point.x >= rect.x && point.x <= rect.x + rect.width && point.y >= rect.y && point.y <= rect.y + rect.height;
}
    
} // namespace Math


