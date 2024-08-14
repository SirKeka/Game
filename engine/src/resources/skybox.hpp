#pragma once
#include "texture_map.hpp"

struct Skybox
{
    TextureMap cubemap;
    GeometryID* g;
    u32 InstanceID;
    u64 RenderFrameNumber;  // Синхронизируется с текущим номером кадра рендерера, когда материал был применен к этому кадру.

    constexpr Skybox(/* args */) : cubemap(), g(), InstanceID(), RenderFrameNumber(INVALID::U64ID) {}
};
