#pragma once
#include "defines.hpp"
#include "math/transform.hpp"
#include "containers/darray.hpp"

struct SkyboxSimpleSceneConfig
{
    MString name;
    MString CubemapName;
};

struct DirectionalLightSimpleSceneConfig
{
    MString name;
    FVec4 colour;
    FVec4 direction;
};

struct PointLightSimpleSceneConfig
{
    MString name;
    FVec4 colour;
    FVec4 position;
    f32 ConstantF;
    f32 linear;
    f32 quadratic;
};

struct MeshSimpleSceneConfig
{
    MString name;
    MString ResourceName;
    Transform transform;
    MString ParentName;       // опционально
};

struct SimpleSceneConfig {
    MString name;
    MString description;
    SkyboxSimpleSceneConfig SkyboxConfig;
    DirectionalLightSimpleSceneConfig DirectionalLightConfig;

    DArray<PointLightSimpleSceneConfig> PointLights;

    DArray<MeshSimpleSceneConfig> meshes;

    void* operator new(u64 size) {
        return MemorySystem::Allocate(size, Memory::Scene);
    }

    void operator delete(void* ptr, u64 size) {
        MemorySystem::Free(ptr, size, Memory::Scene);
    }
};