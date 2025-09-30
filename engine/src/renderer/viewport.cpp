#include "viewport.h"

constexpr Viewport::Viewport(const Rect2D &rect, f32 FOV, f32 NearClip, f32 FarClip, RendererProjectionMatrixType ProjectionMatrixType)
: rect(rect), FOV(FOV), NearClip(NearClip), FarClip(FarClip), ProjectionMatrixType(ProjectionMatrixType), projection(RegenerateProjectionMatrix()) 
{
    if (ProjectionMatrixType == RendererProjectionMatrixType::OrthographicCentered && FOV == 0) {
        MWARN("Viewport::Viewport использует центрированный ортографический тип с полем зрения, равным 0. Поле зрения должно быть ненулевым.");
    }
}

Viewport::~Viewport()
{
    Destroy();
}

bool Viewport::Create(const Rect2D &rect, f32 FOV, f32 NearClip, f32 FarClip, RendererProjectionMatrixType ProjectionMatrixType)
{
    *this = Viewport(rect, FOV, NearClip, FarClip, ProjectionMatrixType);
    return true;
}

void Viewport::Destroy()
{
    MemorySystem::ZeroMem(this, sizeof(Viewport));
}

void Viewport::Resize(const Rect2D &rect)
{
    this->rect = rect;
    projection = RegenerateProjectionMatrix();
}

constexpr Matrix4D Viewport::RegenerateProjectionMatrix()
{
    switch (ProjectionMatrixType) {
    case RendererProjectionMatrixType::Perspective:
        return Matrix4D::MakeFrustumProjection(FOV, (f32)rect.width / rect.height, NearClip, FarClip);
        break;
    case RendererProjectionMatrixType::Orthographic:
        // ПРИМЕЧАНИЕ: возможно, потребуется поменять местами y/w
        return Matrix4D::MakeOrthographicProjection(rect.x, rect.width, rect.height, rect.y, NearClip, FarClip);
    case RendererProjectionMatrixType::OrthographicCentered:
        return Matrix4D::MakeOrthographicProjection(-rect.width * FOV, rect.width * FOV, -rect.height * FOV, rect.height * FOV, NearClip, FarClip);
    default:
        MERROR("Регенерация матрицы перспективной проекции по умолчанию, так как указан недопустимый тип.");
        return Matrix4D::MakeFrustumProjection(FOV, (f32)rect.width / rect.height, NearClip, FarClip);
        break;
    }
}