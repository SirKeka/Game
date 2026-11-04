#pragma once
#include "resources/lighting_structures.h"

class LightSystem
{
public:
    MAPI bool AddDirectional(DirectionalLight* light);
    MAPI bool AddPoint(PointLight* light);

    MAPI bool RemoveDirectional(DirectionalLight* light);
    MAPI bool RemovePoint(PointLight* light);

    MAPI DirectionalLight* GetDirectionalLight();

    MAPI u32 PointLightCount();
    MAPI bool GetPointLights(PointLight** PointLights);
    MAPI bool GetPointLights(PointLight* PointLights);
private:
    #define MAX_POINT_LIGHTS 10U

    DirectionalLight* DirLight;
    PointLight* PointLights[MAX_POINT_LIGHTS];
}; // class LightSystem


