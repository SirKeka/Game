#include "skybox.hpp"
#include "renderer/rendering_system.hpp"
#include "systems/geometry_system.hpp"
#include "systems/shader_system.hpp"
#include "systems/texture_system.hpp"

/*Skybox::~Skybox()
{
    Destroy()
}*/

bool Skybox::Create(const char *CubemapName)
{
    if (!RenderingSystem::TextureMapAcquireResources(&cubemap)) {
        MFATAL("Невозможно получить ресурсы для текстуры кубической карты.");
        return false;
    }
    cubemap.texture = TextureSystem::AcquireCube("skybox", true);  // ЗАДАЧА: имя жестко закодировано.
    auto SkyboxCubeConfig = GeometrySystem::GenerateCubeConfig(10.0f, 10.0f, 10.0f, 1.0f, 1.0f, CubemapName, nullptr);
    // Удалите название материала.
    SkyboxCubeConfig.MaterialName[0] = '\0';
    g = GeometrySystem::Acquire(SkyboxCubeConfig, true);
    RenderFrameNumber = INVALID::U64ID;
    auto SkyboxShader = ShaderSystem::GetShader("Shader.Builtin.Skybox");   // ЗАДАЧА: разрешить настраиваемый шейдер.
    TextureMap* maps[1] = {&cubemap};
    if (!RenderingSystem::ShaderAcquireInstanceResources(SkyboxShader, maps, InstanceID)) {
        MFATAL("Невозможно получить ресурсы шейдера для текстуры скайбокса.");
        return false;
    }
    return true;
}

void Skybox::Destroy()
{
    RenderingSystem::TextureMapReleaseResources(&cubemap);
}
