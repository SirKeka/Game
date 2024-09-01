#include "geometry.hpp"
#include "core/logger.hpp"

constexpr Geometry::Geometry(Geometry &&g)
: 
id(g.id), 
generation(g.generation),
VertexCount(g.VertexCount), 
VertexElementSize(g.VertexElementSize),
VertexBufferOffset(g.VertexBufferOffset),
IndexCount(g.IndexCount),
IndexElementSize(g.IndexElementSize),
IndexBufferOffset(g.IndexBufferOffset) 
{
    g.id = INVALID::ID;
    g.generation = 0;
    g.VertexCount = 0;
    g.VertexElementSize = 0;
    g.VertexBufferOffset = 0;
    g.IndexCount = 0;
    g.IndexElementSize = 0;
    g.IndexBufferOffset = 0;
}

Geometry::~Geometry()
{
    id = INVALID::ID;
    generation = INVALID::ID;
    VertexCount = 0;
    VertexElementSize = 0;
    VertexBufferOffset = 0;
    IndexCount = 0;
    IndexElementSize = 0;
    IndexBufferOffset = 0;
}

Geometry &Geometry::operator=(const Geometry &g)
{
    VertexCount = g.VertexCount;
    VertexElementSize = g.VertexElementSize;
    VertexBufferOffset = g.VertexBufferOffset;
    IndexCount = g.IndexCount;
    IndexElementSize = g.IndexElementSize;
    IndexBufferOffset = g.IndexBufferOffset;
    return *this;
}

Geometry &Geometry::operator=(const Geometry *g)
{
    VertexCount = g->VertexCount;
    VertexElementSize = g->VertexElementSize;
    VertexBufferOffset = g->VertexBufferOffset;
    IndexCount = g->IndexCount;
    IndexElementSize = g->IndexElementSize;
    IndexBufferOffset = g->IndexBufferOffset;
    return *this;
}

void Geometry::SetVertexIndex(u32 VertexCount, u32 VertexElementSize, u32 IndexCount)
{
    this->VertexCount = VertexCount;
    this->VertexElementSize = VertexElementSize;
    this->IndexCount = IndexCount;
    this->IndexElementSize = sizeof(u32);
}

void Geometry::SetVertexData(u32 VertexCount, u32 ElementSize, u64 VertexBufferOffset)
{
    this->VertexCount = VertexCount;
    this->VertexElementSize = ElementSize;
    this->VertexBufferOffset = VertexBufferOffset;
}

void Geometry::SetIndexData(u32 IndexCount, u64 IndexBufferOffset)
{
    this->IndexCount = IndexCount;
    this->IndexElementSize = sizeof(u32);
    this->IndexBufferOffset = IndexBufferOffset;
}

void Geometry::Destroy()
{
    this->~Geometry();
}
