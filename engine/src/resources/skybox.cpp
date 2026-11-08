#include "skybox.h"
#include "core/systems_manager.h"
#include "renderer/rendering_system.h"
#include "systems/geometry_system.h"
#include "systems/shader_system.h"
#include "systems/texture_system.h"

bool Skybox::Create(Config& config)
{
    this->CubemapName = (MString&&)config.CubemapName;

    return true;
}

void Skybox::Destroy()
{
    // Если скайбокс загружен, сначала выгрузите его, а затем уничтожьте.
    if (InstanceID != INVALID::ID) {
        bool result = Unload();
        if (!result) {
            MERROR("Skybox::Destroy() - Не удалось успешно выгрузить скайбокс перед разрушением.");
        }
    }

    MemorySystem::ZeroMem(this, sizeof(Skybox));
}

void Skybox::GenerateHemisphere(u16 rings, u16 segments, f32 radius)
{
    // Считаем общее количество вершин
    /*u32 NumVerticesTopStrip = 3 * segments;
    u32 NumVerticesRegularStrip = 6 * segments;
    config.GConfig.VertexCount = NumVerticesTopStrip + (rings - 1) * NumVerticesRegularStrip;

    DArray<Vertex3D> Vertices(config.GConfig.VertexCount);

    f32 PitchAngle = 90.F / (f32)rings;
    f32 HeadingAngle = 360.F / (f32)segments;

    FVec3 Apex(0.F, radius, 0.F);

    f32 Pitch = -90.F;
    u32 i = 0;

    for (f32 Heading = 0.F ; Heading < 360.F ; Heading += HeadingAngle) {

        Vertex v0(Apex);
        Vertices[i++] = v0; 

        Vector3f Pos1;
        Pos1.InitBySphericalCoords(Radius, Pitch + PitchAngle, Heading + HeadingAngle);
        Vertex v1(Pos1);
        Vertices[i++] = v1;

        Vector3f Pos2;
        Pos2.InitBySphericalCoords(Radius, Pitch + PitchAngle, Heading);
        Vertex v2(Pos2);
        Vertices[i++] = v2;
    }*/
}

bool Skybox::Initialize()
{
    cubemap.FilterMagnify = cubemap.FilterMinify = TextureFilterMode::Linear;
    cubemap.RepeatU = cubemap.RepeatV = cubemap.RepeatW = TextureRepeat::ClampToEdge;

    InstanceID = INVALID::ID;
    if (type == Type::Skybox) {
        GeometryConfig config = GeometrySystem::GenerateCubeConfig(10.F, 10.F, 10.F, 1.F, 1.F, CubemapName.c_str(), nullptr);
        // Удалите название материала.
        config.MaterialName[0] = '\0';
        geometry = GeometrySystem::Acquire(config, true);
    } else {
        // GenerateHemisphere();
    }

    return true;
}

bool Skybox::Load()
{
    cubemap.texture = TextureSystem::AcquireCube(CubemapName.c_str(), true);
    auto renderer = SystemsManager::GetRenderingSystem();
    if (!renderer->TextureMapAcquireResources(&cubemap)) {
        MFATAL("Невозможно получить ресурсы для текстуры кубической карты.");
        return false;
    }

    GeometrySystem::CreateGeometry(geometry);
    RenderFrameNumber= INVALID::U64ID;

    auto SkyboxShader = ShaderSystem::GetShader("Shader.Builtin.Skybox");  // ЗАДАЧА: разрешить настраиваемый шейдер.
    TextureMap* maps[1] = {&cubemap};
    u16 AtlasLocation = SkyboxShader->uniforms[SkyboxShader->InstanceSamplerIndices[0]].index;

    // Количество карт для этого типа известно.
    ShaderInstanceUniformTextureConfig ColourTexture { AtlasLocation, 1, maps };

    ShaderInstanceResourceConfig InstanceResourceConfig { 1, &ColourTexture };

    if (!renderer->ShaderAcquireInstanceResources(SkyboxShader, InstanceResourceConfig, InstanceID)) {
        MFATAL("Невозможно получить ресурсы шейдера для текстуры скайбокса.");
        return false;
    }

    return true;
}

bool Skybox::Unload()
{
    auto SkyboxShader = ShaderSystem::GetShader("Shader.Builtin.Skybox");  // ЗАДАЧА: разрешить настраиваемый шейдер.
    auto renderer = SystemsManager::GetRenderingSystem();
    renderer->ShaderReleaseInstanceResources(SkyboxShader, InstanceID);
    InstanceID = INVALID::ID;
    renderer->TextureMapReleaseResources(&cubemap);

    RenderFrameNumber = INVALID::U64ID;

    if (CubemapName) {
        if (cubemap.texture) {
            TextureSystem::Release(CubemapName.c_str());
            cubemap.texture = nullptr;
        }

        CubemapName.Destroy();
    }

    GeometrySystem::Release(geometry);

    return true;
}


