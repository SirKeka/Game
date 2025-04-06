#pragma once
#include "defines.hpp"
#include "math/transform.h"
#include "math/vertex.h"
#include "core/identifier.h"
#include "resources/geometry.hpp"

struct DebugBox3D {
    u32 UniqueID;
    MString name;
    FVec3 size;
    FVec4 colour;
    Transform xform;

    u32 VertexCount;
    ColourVertex3D* vertices;

    GeometryID geometry;

    constexpr DebugBox3D(const FVec3& size, Transform* parent)
    :
    UniqueID(Identifier::AquireNewID(this)),
    name(),
    size(size),
    colour(FVec4::One()), // Белый цвет по умолчанию
    xform(),
    VertexCount(),
    vertices(nullptr),
    geometry()
    {}
    ~DebugBox3D();

    void SetParent(Transform* parent);
    void SetColour(const FVec4& colour);
    void SetExtents(const Extents3D& extents);

    bool Initialize();
    bool Load();
    bool Unload();

    bool Update();
private:
    void RecalculateExtents(const Extents3D& extents);
    void UpdateVertColour();
};