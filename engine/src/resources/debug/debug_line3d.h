#pragma once

#include "math/transform.h"
#include "resources/geometry.h"

struct MAPI DebugLine3D {
    u32 UniqueID;
    MString name;
    FVec3 Point0;
    FVec3 Point1;
    FVec4 colour;
    Transform xform;

    u32 VertexCount;
    struct ColourVertex3D *vertices;

    Geometry geometry;

    constexpr DebugLine3D() : UniqueID(), name(), Point0(), Point1(), colour(), xform(), VertexCount(), vertices(nullptr), geometry() {}
    constexpr DebugLine3D(const FVec3& Point0, const FVec3& Point1, Transform *parent = nullptr);
    // ~DebugLine3D();

    operator bool() {
        if (!UniqueID || UniqueID == INVALID::ID) {
            return false;
        }
        return true;
    }

    bool Create(const FVec3& Point0, const FVec3& Point1, Transform *parent = nullptr);
    void Destroy();
    void SetParent(Transform *parent);
    void SetColour(const FVec4& colour);
    void SetPoints(const FVec3& Point0, const FVec3& Point1);
    bool Initialize();
    bool Load();
    bool Unload();
    bool Update();

    // void* operator new(u64 size);
    // void operator delete(void* ptr, u64 size);

private:
    void UpdateVertColour();
    void RecalculatePoints();
};
