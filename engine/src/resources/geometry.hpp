#pragma once
#include "containers/mstring.hpp"

#define GEOMETRY_NAME_MAX_LENGTH 256

// Максимальное количество одновременно загружаемых геометрий
// TODO: сделать настраиваемым
#define VULKAN_MAX_GEOMETRY_COUNT 4096

/// @brief Представляет фактическую геометрию в мире.
/// Обычно (но не всегда, в зависимости от использования) в сочетается с материалом.
class Geometry {
private:
    u32 id;
    u32 InternalID;
    u32 generation;
    MString name;
    class Material* material;
public:
    Geometry() : id(), InternalID(), generation(), name(GEOMETRY_NAME_MAX_LENGTH), material(nullptr) {}

    bool Create(u32 VertexCount, const Vertex3D& vertices, u32 IndexCount, const u32& indices);
    void Destroy();
};