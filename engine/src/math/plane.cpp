#include "plane.h"

void Plane::Create(const FVec3 &p1, const FVec3 &norm)
{
    auto n = Normalize(norm);
    x = n.x;
    y = n.y;
    z = n.z;
    distance = Dot(n, p1);
}

f32 Plane::SignedDistance(const FVec3& position)
{
    return Dot(GetNormal(), position) - distance;
}

bool Plane::IntersectsSphere(const FVec3 &center, f32 radius)
{
    // ЗАДАЧА: добавить SIMD
    return SignedDistance(center) > -radius;
}

bool Plane::IntersectsAABB(const FVec3 &center, const FVec3 &extents)
{
    // ЗАДАЧА: добавить SIMD
    f32 r = extents.x * Math::abs(x) +
            extents.y * Math::abs(y) +
            extents.z * Math::abs(z);

    return -r <= SignedDistance(center);
}
