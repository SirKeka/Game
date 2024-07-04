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
        /// @brief Удаляет дубликаты вершин, оставляя только уникальные. 
        /// Оставляет исходный массив вершин нетронутым. Выделяет новый массив в OutVertices. 
        /// Изменяет индексы на месте. Исходный массив вершин должен быть освобожден вызывающей стороной.
        /// @param VertexCount количество вершин в массиве.
        /// @param Vertices исходный массив вершин, подлежащих дедупликации. Не модифицировано.
        /// @param IndexCount количество индексов в массиве.
        /// @param indices массив индексов. Изменяется на месте по мере удаления вершин.
        /// @param OutVertexCount указатель для хранения окончательного количества вершин.
        /// @param OutVertices указатель для хранения массива дедуплицированных вершин.
        void DeduplicateVertices(u32 VertexCount, Vertex3D* vertices, u32 IndexCount, u32* indices, u32& OutVertexCount, Vertex3D** OutVertices);
    } // namespace Geometry
} // namespace Math


