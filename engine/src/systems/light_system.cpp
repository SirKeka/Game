#include "light_system.hpp"

#include "core/systems_manager.hpp"
#include "core/mmemory.hpp"

#define MAX_POINT_LIGHTS 10U

struct LightSystemState {
    DirectionalLight* DirLight;
    PointLight* PointLights[MAX_POINT_LIGHTS];
};

bool LightSystem::Initialize(u64 &MemoryRequirement, void *memory, void *config)
{
    MemoryRequirement = sizeof(LightSystemState);
    if (!memory) {
        return true;
    }

    // ПРИМЕЧАНИЕ: выполните здесь настройку/инициализацию.
    return true;
}

void LightSystem::Shutdown()
{

}

bool LightSystem::AddDirectional(DirectionalLight *light)
{
    if (!light) {
        return false;
    }
    auto state = reinterpret_cast<LightSystemState*>(SystemsManager::GetState(MSystem::Light));
    state->DirLight = light;
    return true;
}

bool LightSystem::AddPoint(PointLight *light)
{
    if (!light) {
        return false;
    }
    auto state = reinterpret_cast<LightSystemState*>(SystemsManager::GetState(MSystem::Light));
    for (u32 i = 0; i < MAX_POINT_LIGHTS; ++i) {
        if (!state->PointLights[i]) {
            state->PointLights[i] = light;
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
    auto state = reinterpret_cast<LightSystemState*>(SystemsManager::GetState(MSystem::Light));
    if (state->DirLight == light) {
        state->DirLight = nullptr;
        return true;
    }

    return false;
}

bool LightSystem::RemovePoint(PointLight *light)
{
    if (!light) {
        return false;
    }
    auto state = reinterpret_cast<LightSystemState*>(SystemsManager::GetState(MSystem::Light));
    for (u32 i = 0; i < MAX_POINT_LIGHTS; ++i) {
        if (state->PointLights[i] == light) {
            state->PointLights[i] = nullptr;
            return true;
        }
    }

    MERROR("LightSystem::RemovePoint не имеет источника света, соответствующего переданному, поэтому его нельзя удалить.");
    return false;
}

DirectionalLight *LightSystem::GetDirectionalLight()
{
    auto state = reinterpret_cast<LightSystemState*>(SystemsManager::GetState(MSystem::Light));
    return state->DirLight;
}

u32 LightSystem::PointLightCount()
{
    auto state = reinterpret_cast<LightSystemState*>(SystemsManager::GetState(MSystem::Light));
    u32 count = 0;
    for (u32 i = 0; i < MAX_POINT_LIGHTS; ++i) {
        if (state->PointLights[i]) {
            count++;
        }
    }

    return count;
}

bool LightSystem::GetPointLights(PointLight *PointLights)
{
    if (!PointLights) {
        return false;
    }
    auto state = reinterpret_cast<LightSystemState*>(SystemsManager::GetState(MSystem::Light));
    for (u32 i = 0, j = 0; i < MAX_POINT_LIGHTS; ++i) {
        if (state->PointLights[i]) {
            PointLights[j] = *(state->PointLights[i]);
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