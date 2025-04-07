#pragma once
#include "defines.hpp"
#include "math/transform.h"
#include "resources/geometry.hpp"

struct MAPI DebugLine3D {
    u32 UniqueID;
    MString name;
    FVec3 Point0;
    FVec3 Point1;
    FVec4 colour;
    Transform xform;

    u32 VertexCount;
    struct ColourVertex3D *vertices;

    GeometryID geometry;

    constexpr DebugLine3D(const FVec3& Point0, const FVec3& Point1, Transform *parent);
    // ~DebugLine3D();

    operator bool() {
        if (!UniqueID || UniqueID == INVALID::ID) {
            return false;
        }
        return true;
    }

    void Destroy();
    void SetParent(Transform *parent);
    void SetColour(const FVec4& colour);
    void SetPoints(const FVec3& Point0, const FVec3& Point1);
    bool Initialize();
    bool Load();
    bool Unload();
    bool Update();

private:
    void UpdateVertColour();
    void RecalculatePoints();
};
