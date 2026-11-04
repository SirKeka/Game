#pragma once

#include "math/transform.h"
#include "containers/darray.h"

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
    f32 ShadowDistance;
    f32 ShadowFadeDistance;
    f32 ShadowSplitMult;
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

struct MeshConfig
{
    MString name;
    MString ResourceName;
    MString ParentName;       // опционально
    Transform transform;
    u16 GeometryCount;
    struct GeometryConfig* GConfigs;
};

struct TerrainSimpleSceneConfig 
{
    MString name;
    MString ResourceName;
    Transform xform;
};

struct SimpleSceneConfig 
{
    MString name;
    MString description;
    SkyboxSimpleSceneConfig SkyboxConfig;
    DirectionalLightSimpleSceneConfig DirectionalLightConfig;

    DArray<PointLightSimpleSceneConfig> PointLights;
    DArray<MeshConfig> meshes;
    DArray<TerrainSimpleSceneConfig> terrains;

    void* operator new(u64 size) { return MemorySystem::Allocate(size, Memory::Scene); }

    void operator delete(void* ptr, u64 size) { MemorySystem::Free(ptr, size, Memory::Scene); }
};