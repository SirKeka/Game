#include "geometry.hpp"
#include "core/logger.hpp"
//#include "renderer/vulkan/vulkan_api.hpp"

Geometry::Geometry() 
: 
id(INVALID_ID), 
generation(INVALID_ID), 
//name(/*GEOMETRY_NAME_MAX_LENGTH*/), 
VertexCount(), 
VertexElementSize(),
VertexBufferOffset(),
IndexCount(),
IndexElementSize(),
IndexBufferOffset()/*,
material(nullptr)*/ {}

Geometry::Geometry(u32 VertexCount, u32 VertexBufferOffset, u32 IndexCount, u32 IndexBufferOffset)
:
id(INVALID_ID), 
generation(INVALID_ID), 
//name(/*GEOMETRY_NAME_MAX_LENGTH*/),
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
//name(g.name), 
VertexCount(g.VertexCount), 
VertexElementSize(g.VertexElementSize),
VertexBufferOffset(g.VertexBufferOffset),
IndexCount(g.IndexCount),
IndexElementSize(g.IndexElementSize),
IndexBufferOffset(g.IndexBufferOffset)/*,
material(g.material)*/ {}

Geometry::Geometry(Geometry &&g)
: 
id(g.id), 
//InternalID(g.InternalID), 
//generation(g.generation), 
//name(g.name), 
VertexCount(g.VertexCount), 
VertexElementSize(g.VertexElementSize),
VertexBufferOffset(g.VertexBufferOffset),
IndexCount(g.IndexCount),
IndexElementSize(g.IndexElementSize),
IndexBufferOffset(g.IndexBufferOffset)/*,
material(g.material)*/
{
    g.id = INVALID_ID;
    //g.InternalID = INVALID_ID;
    //g.generation = INVALID_ID;
    // g.name
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

void Geometry::SetVertexData(u32 VertexCount, u32 VertexBufferOffset) 
{
    this->VertexCount = VertexCount;
    this->VertexElementSize = sizeof(Vertex3D);
    this->VertexBufferOffset = VertexBufferOffset;
}

void Geometry::SetIndexData(u32 IndexCount, u32 IndexBufferOffset)
{
    this->IndexCount = IndexCount;
    this->IndexElementSize = sizeof(u32);
    this->IndexBufferOffset = IndexBufferOffset;
}

void Geometry::Destroy()
{
    this->~Geometry();
}
