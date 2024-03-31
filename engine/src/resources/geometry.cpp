#include "geometry.hpp"
#include "core/logger.hpp"
//#include "renderer/vulkan/vulkan_api.hpp"

Geometry::Geometry() 
: 
id(INVALID_ID), 
generation(INVALID_ID), 
//name(/*GEOMETRY_NAME_MAX_LENGTH*/), 
VertexCount(), 
VertexSize(),
VertexBufferOffset(),
IndexCount(),
IndexSize(),
IndexBufferOffset()/*,
material(nullptr)*/ {}

Geometry::Geometry(u32 VertexCount, u32 VertexBufferOffset, u32 IndexCount, u32 IndexBufferOffset)
:
id(INVALID_ID), 
generation(INVALID_ID), 
//name(/*GEOMETRY_NAME_MAX_LENGTH*/),
VertexCount(VertexCount), 
VertexSize(VertexCount * sizeof(Vertex3D)), 
VertexBufferOffset(VertexBufferOffset), 
IndexCount(IndexCount), 
IndexSize(IndexCount * sizeof(u32)), 
IndexBufferOffset(IndexBufferOffset) {}

Geometry::Geometry(const Geometry &g) 
: 
id(g.id), 
generation(g.generation), 
//name(g.name), 
VertexCount(g.VertexCount), 
VertexSize(g.VertexSize),
VertexBufferOffset(g.VertexBufferOffset),
IndexCount(g.IndexCount),
IndexSize(g.IndexSize),
IndexBufferOffset(g.IndexBufferOffset)/*,
material(g.material)*/ {}

Geometry::Geometry(Geometry &&g)
: 
id(g.id), 
//InternalID(g.InternalID), 
//generation(g.generation), 
//name(g.name), 
VertexCount(g.VertexCount), 
VertexSize(g.VertexSize),
VertexBufferOffset(g.VertexBufferOffset),
IndexCount(g.IndexCount),
IndexSize(g.IndexSize),
IndexBufferOffset(g.IndexBufferOffset)/*,
material(g.material)*/
{
    g.id = INVALID_ID;
    //g.InternalID = INVALID_ID;
    //g.generation = INVALID_ID;
    // g.name
    g.VertexCount = 0;
    g.VertexSize = 0;
    g.VertexBufferOffset = 0;
    g.IndexCount = 0;
    g.IndexSize = 0;
    g.IndexBufferOffset = 0;
}

Geometry &Geometry::operator=(const Geometry &g)
{
    VertexCount = g.VertexCount;
    VertexSize = g.VertexSize;
    VertexBufferOffset = g.VertexBufferOffset;
    IndexCount = g.IndexCount;
    IndexSize = g.IndexSize;
    IndexBufferOffset = g.IndexBufferOffset;
    return *this;
}

Geometry &Geometry::operator=(const Geometry *g)
{
    VertexCount = g->VertexCount;
    VertexSize = g->VertexSize;
    VertexBufferOffset = g->VertexBufferOffset;
    IndexCount = g->IndexCount;
    IndexSize = g->IndexSize;
    IndexBufferOffset = g->IndexBufferOffset;
    return *this;
}

void Geometry::SetVertexData(u32 VertexCount, u32 VertexBufferOffset) 
{
    this->VertexCount = VertexCount;
    this->VertexSize = VertexCount * sizeof(Vertex3D);
    this->VertexBufferOffset = VertexBufferOffset;
}

void Geometry::SetIndexData(u32 IndexCount, u32 IndexBufferOffset)
{
    this->IndexCount = IndexCount;
    this->IndexSize = IndexCount * sizeof(u32);
    this->IndexBufferOffset = IndexBufferOffset;
}

void Geometry::Destroy()
{
    id = INVALID_ID;
    //generation = INVALID_ID;
    //name.Destroy();
    VertexCount = 0;
    VertexSize = 0;
    VertexBufferOffset = 0;
    IndexCount = 0;
    IndexSize = 0;
    IndexBufferOffset = 0;
}
