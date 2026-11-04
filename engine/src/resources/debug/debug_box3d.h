#pragma once

#include "math/transform.h"
#include "math/vertex.h"
#include "core/identifier.h"
#include "resources/geometry.h"

struct MAPI DebugBox3D {
    Identifier id;
    MString name;
    FVec3 size;
    FVec4 colour;
    Transform xform;
    bool IsDirty;

    u32 VertexCount;
    ColourVertex3D* vertices;

    Geometry geometry;

    constexpr DebugBox3D() = default;
    DebugBox3D(FVec3 size, Transform* parent = nullptr)
    : id(), name(), size(size),
    colour(FVec4::One()), // Белый цвет по умолчанию
    xform(), IsDirty(true), VertexCount(), vertices(nullptr), geometry()
    {
        if (parent) {
            xform.SetParent(parent);
        }
    }
    // ~DebugBox3D();
    void Destroy();

    explicit operator bool() const {
        if (id == INVALID::ID) { return false; }
        return true;
    }

    void SetColour(FVec4 colour);
    void SetExtents(const Extents3D& extents);
    void SetParent(Transform* parent);
    void SetPoints(FVec4* points);
    void RenderFramePrepare(const struct FrameData& rFrameData);

    bool Initialize();
    bool Load();
    bool Unload();

    bool Update();
private:
    void RecalculateExtents(const Extents3D& extents);
    void UpdateVertColour();
};