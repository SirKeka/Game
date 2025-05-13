#pragma once
#include <defines.hpp>

/// @brief Представляет фактическую геометрию в мире.
/// Обычно (но не всегда, в зависимости от использования) в сочетается с материалом.
struct VulkanGeometry {
    u32 id;
    u32 generation;
    //char name[GEOMETRY_NAME_MAX_LENGTH];

    u32 VertexCount;
    u32 VertexElementSize;
    u64 VertexBufferOffset;
    u32 IndexCount;
    u32 IndexElementSize;
    u64 IndexBufferOffset;

    constexpr VulkanGeometry() : id(INVALID::ID), generation(), VertexCount(), VertexElementSize(), VertexBufferOffset(), IndexCount(), IndexElementSize(), IndexBufferOffset() {}
    constexpr VulkanGeometry(u32 VertexCount, u32 VertexElementSize, u32 IndexCount, u32 IndexElementSize): id(), generation(), VertexCount(VertexCount), VertexElementSize(VertexElementSize), VertexBufferOffset(), IndexCount(IndexCount), IndexElementSize(IndexElementSize), IndexBufferOffset() {}

    ~VulkanGeometry() { Destroy(); }

    void Destroy() {
        id = generation = INVALID::ID;
        VertexCount = VertexElementSize = IndexCount =  IndexElementSize = 0;
        VertexBufferOffset = IndexBufferOffset = 0;
    }

};
