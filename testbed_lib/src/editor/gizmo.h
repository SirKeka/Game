#pragma once

#include <resources/geometry.h>
#include "math/transform.h"
#include "math/plane.h"

#ifdef _DEBUG
#include <resources/debug/debug_line3d.h>
#endif

class Camera;
struct Ray;

struct MAPI Gizmo {
    enum Mode {
        NONE = 0,
        MOVE = 1,
        ROTATE = 2,
        SCALE = 3,
        MAX = SCALE,
    };

    enum class InteractionType { None, MouseHover, MouseDown, MouseDrag, MouseUp, Cancel };

    struct ModeData {
        u32 VertexCount;
        ColourVertex3D* vertices;

        u32 IndexCount;
        u32* indices;

        Geometry geometry;

        u32 ExtentsCount;
        Extents3D* ModeExtents;

        u8 CurrentAxisIndex;
        Plane InteractionPlane;
        Plane InteractionPlaneBack;

        FVec3 InteractionStartPosition;
        FVec3 LastInteractionPosition;
    };

    enum Orientation {
        /// @brief Операции преобразования Гизмо выполняются относительно глобального преобразования.
        Global = 0,
        /// @brief Операции преобразования Гизмо выполняются относительно локального преобразования.
        Local = 1,
        /// @brief Операции преобразования Гизмо выполняются относительно текущего представления.
        // View = 2,
        Max = Local
    };
    
    Transform xform;            // Преобразование гизмо.
    Transform* SelectedXform;   // Указатель на преобразование текущего выбранного объекта. Значение NULL, если ничего не выбрано.
    Mode mode;                  // Текущий режим гизмо.
    f32 ScaleScalar;            // Используется для поддержания постоянного размера устройства на экране независимо от расстояния до камеры.
    Orientation orientation;    // Указывает ориентацию операции преобразования в редакторе.
    ModeData modeData[MAX + 1]; // Данные для каждого режима гизмо.
    InteractionType interaction;

#ifdef _DEBUG
    DebugLine3D PlaneNormalLine;
#endif

    constexpr Gizmo() : xform(), SelectedXform(nullptr), mode(), ScaleScalar(), orientation(Local), modeData(), interaction() {}

    bool Create();
    void Destroy();

    bool Initialize();
    bool Load();
    bool Unload();

    void Refresh();
    Orientation GetOrientation();
    void SetOrientation(Orientation orientation);
    void SetSelectedTransform(Transform* xform = nullptr);

    void Update();

    void SetMode(Mode mode);

    void InteractionBegin(Camera* camera, Ray& ray, InteractionType iType);
    void InteractionEnd();
    void HandleInteraction(Camera* camera, Ray& ray, InteractionType iType);

private:
    void CreateGizmoModeNone();
    void CreateGizmoModeMove();
    void CreateGizmoModeScale();
    void CreateGizmoModeRotate();
};


