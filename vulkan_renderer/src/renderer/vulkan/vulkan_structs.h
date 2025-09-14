#pragma once
#include <defines.h>

/// @brief Представляет фактическую геометрию в мире.
/// Обычно (но не всегда, в зависимости от использования) в сочетается с материалом.
struct VulkanGeometry {
    u32 id;
    u32 generation;

    u64 VertexBufferOffset;
    u64 IndexBufferOffset;

    constexpr VulkanGeometry() : id(INVALID::ID), generation(),VertexBufferOffset(),IndexBufferOffset() {}
    constexpr VulkanGeometry(u32 VertexCount, u32 VertexElementSize, u32 IndexCount, u32 IndexElementSize): id(), generation(),VertexBufferOffset(), IndexBufferOffset() {}

    ~VulkanGeometry() { Destroy(); }

    void Destroy() {
        id = generation = INVALID::ID;
        VertexBufferOffset = IndexBufferOffset = 0;
    }

};
