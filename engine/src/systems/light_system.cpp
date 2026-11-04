#include "light_system.h"

#include "core/systems_manager.h"
#include "core/memory_system.h"

bool LightSystem::AddDirectional(DirectionalLight *light)
{
    if (!light) {
        return false;
    }

    DirLight = light;
    return true;
}

bool LightSystem::AddPoint(PointLight *light)
{
    if (!light) {
        return false;
    }

    for (u32 i = 0; i < MAX_POINT_LIGHTS; ++i) {
        if (!PointLights[i]) {
            PointLights[i] = light;
            return true;
        }
    }

    MERROR("LightSystem::AddPoint уже имеет максимум %u ламп. Свет не добавлен.", MAX_POINT_LIGHTS);
    return false;
}

bool LightSystem::RemoveDirectional(DirectionalLight *light)
{
    if (!light) {
        return false;
    }

    if (DirLight == light) {
        DirLight = nullptr;
        return true;
    }

    return false;
}

bool LightSystem::RemovePoint(PointLight *light)
{
    if (!light) {
        return false;
    }

    for (u32 i = 0; i < MAX_POINT_LIGHTS; ++i) {
        if (PointLights[i] == light) {
            PointLights[i] = nullptr;
            return true;
        }
    }

    MERROR("LightSystem::RemovePoint не имеет источника света, соответствующего переданному, поэтому его нельзя удалить.");
    return false;
}

DirectionalLight *LightSystem::GetDirectionalLight()
{

    return DirLight;
}

u32 LightSystem::PointLightCount()
{
    u32 count = 0;
    for (u32 i = 0; i < MAX_POINT_LIGHTS; ++i) {
        if (PointLights[i]) {
            count++;
        }
    }

    return count;
}

bool LightSystem::GetPointLights(PointLight **PointLights)
{
    if (!PointLights) {
        return false;
    }

    for (u32 i = 0, j = 0; i < MAX_POINT_LIGHTS; ++i) {
        auto PointLight = this->PointLights[i];
        if (PointLight) {
            PointLights[j] = PointLight;
            j++;
        }
    }

    return true;
}

bool LightSystem::GetPointLights(PointLight *PointLights)
{
    if (!PointLights) {
        return false;
    }

    for (u32 i = 0, j = 0; i < MAX_POINT_LIGHTS; ++i) {
        if (this->PointLights[i]) {
            PointLights[j] = *(this->PointLights[i]);
            j++;
        }
    }

    return true;
}

void *DirectionalLight::operator new(u64 size)
{
    return MemorySystem::Allocate(size, Memory::Scene);
}

void DirectionalLight::operator delete(void *ptr, u64 size)
{
    MemorySystem::Free(ptr, size, Memory::Scene);
}