#pragma once
#include "resources/lighting_structures.h"

namespace LightSystem
{
    bool Initialize(u64& MemoryRequirement, void* memory, void* config);
    void Shutdown();

    MAPI bool AddDirectional(DirectionalLight* light);
    MAPI bool AddPoint(PointLight* light);

    MAPI bool RemoveDirectional(DirectionalLight* light);
    MAPI bool RemovePoint(PointLight* light);

    MAPI DirectionalLight* GetDirectionalLight();

    MAPI u32 PointLightCount();
    MAPI bool GetPointLights(PointLight* PointLights);
} // namespace name


