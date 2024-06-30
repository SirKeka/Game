#pragma once

#include "vertex.hpp"

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
        void GenerateTangents(u32 VertexCount, Vertex3D* vertices, u32 IndexCount, u32* indices);
    } // namespace Geometry
} // namespace Math


