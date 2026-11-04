#include "geometry.h"
#include "core/systems_manager.h"
#include "renderer/rendering_system.h"

void GeometryRenderData::CopyData(const Geometry* geometry) {
        if (geometry) {
        material           = geometry->material;
        VertexCount        = geometry->VertexCount;
        VertexElementSize  = geometry->VertexElementSize;
        VertexBufferOffset = geometry->VertexBufferOffset;
        IndexCount         = geometry->IndexCount;
        IndexElementSize   = geometry->IndexElementSize;
        IndexBufferOffset  = geometry->IndexBufferOffset;
        }
}