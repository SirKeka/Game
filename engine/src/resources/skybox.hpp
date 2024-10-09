#pragma once
#include "texture_map.hpp"

struct MAPI Skybox
{
    TextureMap cubemap;
    struct GeometryID* g;
    u32 InstanceID;
    u64 RenderFrameNumber;  // Синхронизируется с текущим номером кадра рендерера, когда материал был применен к этому кадру.

    // constexpr Skybox(): cubemap(), g(), InstanceID(), RenderFrameNumber() {}
    // ~Skybox();

    bool Create(const char* CubemapName);
    void Destroy();
};
