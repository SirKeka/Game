#pragma once
#include "renderer_types.h"
#include "math/matrix4d.h"

struct MAPI Viewport
{
    /// @brief размеры этой области просмотра, позиция x/y, z/w - это ширина/высота.
    Rect2D rect;
    f32 FOV;
    f32 NearClip;
    f32 FarClip;
    RendererProjectionMatrixType ProjectionMatrixType;
    Matrix4D projection;

    constexpr Viewport() = default;
    constexpr Viewport(const Rect2D& rect, f32 FOV, f32 NearClip, f32 FarClip, RendererProjectionMatrixType ProjectionMatrixType); 
    ~Viewport();

    bool Create(const Rect2D& rect, f32 FOV, f32 NearClip, f32 FarClip, RendererProjectionMatrixType ProjectionMatrixType);
    void Destroy();

    void Resize(const Rect2D& rect);
private:
    constexpr Matrix4D RegenerateProjectionMatrix();
};
