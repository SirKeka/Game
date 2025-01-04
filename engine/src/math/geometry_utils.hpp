#pragma once

#include "vertex.hpp"

struct GeometryConfig;

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
        /// @param geometry конфигурация геометрии содержащая информацию о вершинах и индексах
        /// @param OutVertexCount указатель для хранения окончательного количества вершин.
        /// @param OutVertices указатель для хранения массива дедуплицированных вершин.
        void DeduplicateVertices(GeometryConfig& geometry, u32& OutVertexCount, Vertex3D** OutVertices);
    } // namespace Geometry
} // namespace Math


