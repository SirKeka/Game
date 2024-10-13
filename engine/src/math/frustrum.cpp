#include "frustrum.hpp"

void Frustrum::Create(const FVec3 &position, const FVec3 &forward, const FVec3 &right, const FVec3 &up, f32 aspect, f32 fov, f32 near, f32 far)
{
    f32 HalfV = far * Math::tan(fov * 0.5F);
    f32 HalfH = HalfV * aspect;
    auto ForwardFar = forward * far;

    sides[Top].Create(forward * near + position, forward); 
    sides[Bottom].Create(position + ForwardFar, forward * -1.F); 
    sides[Right].Create(position, Cross(up, ForwardFar + right * HalfH)); 
    sides[Left].Create(position, Cross(ForwardFar - right * HalfH, up)); 
    sides[Back].Create(position, Cross(right, ForwardFar - up * HalfV)); 
    sides[Front].Create(position, Cross(ForwardFar + up * HalfV, right)); 
}

bool Frustrum::IntersectsSphere(const FVec3 &center, f32 radius)
{
    for (u8 i = 0; i < 6; ++i) {
        if (!sides[i].IntersectsSphere(center, radius)) {
            return false;
        }
    }
    return true;
}

bool Frustrum::IntersectsAABB(const FVec3 &center, const FVec3 &extents)
{
    for (u8 i = 0; i < 6; ++i) {
        if (!sides[i].IntersectsAABB(center, extents)) {
            return false;
        }
    }
    return true;
}
