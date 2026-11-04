#pragma once

#include "core/identifier.h"
#include "math/transform.h"
#include "resources/geometry.h"

struct MAPI DebugLine3D {
    Identifier id;
    MString name;
    FVec3 Point0;
    FVec3 Point1;
    FVec4 colour;
    Transform xform;
    bool IsDirty;
    
    u32 VertexCount;
    struct ColourVertex3D *vertices;

    Geometry geometry;

    constexpr DebugLine3D() = default;
    DebugLine3D(FVec3 Point0, FVec3 Point1, Transform *parent = nullptr);
    // ~DebugLine3D();

    operator bool() {
        if (id == INVALID::ID) {
            return false;
        }
        return true;
    }

    bool Create(FVec3 Point0, FVec3 Point1, Transform *parent = nullptr);
    void Destroy();
    void SetParent(Transform *parent);
    void SetColour(FVec4 colour);
    void SetPoints(FVec3 Point0, FVec3 Point1);
    bool Initialize();
    bool Load();
    void RenderFramePrepare(const struct FrameData& rFrameData);
    bool Unload();
    bool Update();

    // void* operator new(u64 size);
    // void operator delete(void* ptr, u64 size);

private:
    void UpdateVertColour();
    void RecalculatePoints();
};
