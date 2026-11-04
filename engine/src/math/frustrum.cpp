#include "frustrum.h"
#include "renderer/camera.h"
#include "renderer/viewport.h"

constexpr Frustum Frustum::Create(const FVec3 &position, const FVec3 &forward, const FVec3 &right, const FVec3 &up, f32 aspect, f32 fov, f32 near, f32 far)
{
    Frustum f;
    f32 HalfV = far * Math::tan(fov * 0.5F);
    f32 HalfH = HalfV * aspect;
    auto ForwardFar = forward * far;
    FVec3 RightHalfH = right * HalfH;
    FVec3 UpHalfV = up * HalfV;

    f.sides[Top].Create(forward * near + position, forward); 
    f.sides[Bottom].Create(position + ForwardFar, forward * -1.F); 
    f.sides[Right].Create(position, Cross(up, ForwardFar + RightHalfH)); 
    f.sides[Left].Create(position, Cross(ForwardFar - RightHalfH, up)); 
    f.sides[Back].Create(position, Cross(right, ForwardFar - UpHalfV)); 
    f.sides[Front].Create(position, Cross(ForwardFar + UpHalfV, right));
    return f; 
}

constexpr Frustum Frustum::Create(Camera *camera, const Viewport *viewport)
{
    return Frustum::Create(camera->GetPosition(), camera->Forward(), camera->Right(), camera->Up(), (f32)viewport->rect.width/viewport->rect.height, viewport->FOV, viewport->NearClip, viewport->FarClip);
}

Frustum Frustum::Create(const Matrix4D &ViewProjection)
{
    Frustum f;

    // Получите обратную матрицу проекции вида.
    Matrix4D inv = Matrix4D::MakeInverse(ViewProjection);
    f32 *md = inv.data;

    // Извлеките строки.
    const FVec4& v1 = reinterpret_cast<const FVec4&>(md[0]);  // FVec4 mat0 {md[0], md[1], md[2], md[3]};
    const FVec4& v2 = reinterpret_cast<const FVec4&>(md[4]);  // FVec4 mat1 {md[4], md[5], md[6], md[7]};
    const FVec4& v3 = reinterpret_cast<const FVec4&>(md[8]);  // FVec4 mat2 {md[8], md[9], md[10], md[11]};
    const FVec4& v4 = reinterpret_cast<const FVec4&>(md[12]); // FVec4 mat3 {md[12], md[13], md[14], md[15]};

    // Рассчитайте плоскости проекции и нормализуйте их, включая расстояния.
    FVec4 sides[6];
    sides[Left]   = Normalize(v4 + v1);
    sides[Right]  = Normalize(v4 - v1);
    sides[Top]    = Normalize(v4 - v2);
    sides[Bottom] = Normalize(v4 + v2);
    sides[Back]   = Normalize(v4 + v3);
    sides[Front]  = Normalize(v4 - v3);

    // Извлеките нормали и расстояния до плоскостей.
    for (u32 i = 0; i < 6; ++i) {
        f.sides[i].Normal() = sides[i];
        f.sides[i].distance = sides[i].w;
    }

    return f;
}

void Frustum::CornerPointsWorldSpace(const Matrix4D &ProjectionView, FVec4 *corners)
{
    auto InverseViewProj = Matrix4D::MakeInverse(ProjectionView);

    corners[0] = FVec4(-1.F, -1.F, 0.F, 1.F);
    corners[1] = FVec4( 1.F, -1.F, 0.F, 1.F);
    corners[2] = FVec4( 1.F,  1.F, 0.F, 1.F);
    corners[3] = FVec4(-1.F,  1.F, 0.F, 1.F);

    corners[4] = FVec4(-1.F, -1.F, 1.F, 1.F);
    corners[5] = FVec4( 1.F, -1.F, 1.F, 1.F);
    corners[6] = FVec4( 1.F,  1.F, 1.F, 1.F);
    corners[7] = FVec4(-1.F,  1.F, 1.F, 1.F);

    for (u32 i = 0; i < 8; ++i) {
        FVec4 point = InverseViewProj * corners[i];
        corners[i] = point / point.w;
        point = InverseViewProj * corners[++i];
        corners[i] = point / point.w;
    }
}

bool Frustum::IntersectsSphere(const FVec3 &center, f32 radius)
{
    for (u8 i = 0; i < 6; ++i) {
        if (!sides[i].IntersectsSphere(center, radius)) {
            return false;
        }
    }
    return true;
}

bool Frustum::IntersectsAABB(const FVec3 &center, const FVec3 &extents) const
{
    for (u8 i = 0; i < 6; ++i) {
        if (!sides[i].IntersectsAABB(center, extents) || !sides[i += 1].IntersectsAABB(center, extents)) {
            return false;
        }
    }
    return true;
}
