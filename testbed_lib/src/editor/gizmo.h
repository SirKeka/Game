#pragma once

#include <resources/geometry.h>
#include "math/transform.h"

struct MAPI Gizmo {
    enum Mode {
        None = 0,
        Move = 1,
        Rotate = 2,
        Scale = 3,
        MAX = Scale,
    };

    struct ModeData {
        u32 VertexCount;
        ColourVertex3D* vertices;

        u32 IndexCount;
        u32* indices;

        Geometry geometry;
    };

    
    Transform xform;            // Преобразование гизмо.
    Mode mode;                  // Текущий режим гизмо.
    ModeData modeData[MAX + 1]; // Данные для каждого режима гизмо.

    constexpr Gizmo() : xform(), mode(), modeData() {}

    bool Create();
    void Destroy();

    bool Initialize();
    bool Load();
    bool Unload();

    void Update();

    void ModeSet(Mode mode);

private:
    void CreateGizmoModeNone();
    void CreateGizmoModeMove();
    void CreateGizmoModeScale();
    void CreateGizmoModeRotate();
};


