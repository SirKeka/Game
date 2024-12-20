#pragma once

#include "defines.hpp"

#include "math/vector4d_fwd.hpp"

struct DirectionalLight {
    FVec4 colour;
    FVec4 direction;  // ignore w
};

struct PointLight {
    FVec4 colour;
    FVec4 position;  // ignore w
    // Обычно 1, убедитесь, что знаменатель никогда не становится меньше 1
    f32 ConstantF;
    // Линейно уменьшает интенсивность света
    f32 linear;
    // Заставляет свет падать медленнее на больших расстояниях.
    f32 quadratic;
    f32 padding;
};

namespace LightSystem
{
    bool Initialize(u64& MemoryRequirement, void* memory, void* config);
    void Shutdown();

    MAPI bool AddDirectional(DirectionalLight* light);
    MAPI bool AddPoint(PointLight* light);

    MAPI bool RemoveDirectional(DirectionalLight* light);
    MAPI bool RemovePoint(PointLight* light);

    MAPI DirectionalLight* GetDirectionalLight();

    MAPI i32 PointLightCount();
    MAPI bool GetPointLights(PointLight* PointLights);
} // namespace name


