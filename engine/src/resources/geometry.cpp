#include "geometry.hpp"
#include "core/logger.hpp"

Geometry::Geometry() 
: 
id(INVALID_ID), 
generation(INVALID_ID), 
VertexCount(), 
VertexElementSize(),
VertexBufferOffset(),
IndexCount(),
IndexElementSize(),
IndexBufferOffset() {}

Geometry::Geometry(u32 VertexCount, u64 VertexBufferOffset, u32 IndexCount, u64 IndexBufferOffset)
:
id(INVALID_ID), 
generation(INVALID_ID), 
VertexCount(VertexCount), 
VertexElementSize(VertexCount * sizeof(Vertex3D)), 
VertexBufferOffset(VertexBufferOffset), 
IndexCount(IndexCount), 
IndexElementSize(IndexCount * sizeof(u32)), 
IndexBufferOffset(IndexBufferOffset) {}

Geometry::Geometry(const Geometry &g) 
: 
id(g.id), 
generation(g.generation), 
VertexCount(g.VertexCount), 
VertexElementSize(g.VertexElementSize),
VertexBufferOffset(g.VertexBufferOffset),
IndexCount(g.IndexCount),
IndexElementSize(g.IndexElementSize),
IndexBufferOffset(g.IndexBufferOffset) {}

Geometry::Geometry(Geometry &&g)
: 
id(g.id), 

VertexCount(g.VertexCount), 
VertexElementSize(g.VertexElementSize),
VertexBufferOffset(g.VertexBufferOffset),
IndexCount(g.IndexCount),
IndexElementSize(g.IndexElementSize),
IndexBufferOffset(g.IndexBufferOffset) 
{
    g.id = INVALID_ID;
    g.VertexCount = 0;
    g.VertexElementSize = 0;
    g.VertexBufferOffset = 0;
    g.IndexCount = 0;
    g.IndexElementSize = 0;
    g.IndexBufferOffset = 0;
}

Geometry::~Geometry()
{
    id = INVALID_ID;
    generation = INVALID_ID;
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
