#include "skybox.hpp"
#include "renderer/rendering_system.hpp"
#include "systems/geometry_system.hpp"
#include "systems/shader_system.hpp"
#include "systems/texture_system.hpp"

bool Skybox::Create(Config& config)
{
    this->config = static_cast<Config&&>(config);

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

bool Skybox::Initialize()
{
    TextureMap* CubeMap = &cubemap;
    CubeMap->FilterMagnify = CubeMap->FilterMinify = TextureFilter::ModeLinear;
    CubeMap->RepeatU = CubeMap->RepeatV = CubeMap->RepeatW = TextureRepeat::ClampToEdge;
    CubeMap->use = TextureUse::Cubemap;

    InstanceID = INVALID::ID;

    config.GConfig = GeometrySystem::GenerateCubeConfig(10.F, 10.F, 10.F, 1.F, 1.F, config.CubemapName.c_str(), nullptr);
    // Удалите название материала.
    config.GConfig.MaterialName[0] = 0;

    return true;
}

bool Skybox::Load()
{
    cubemap.texture = TextureSystem::AcquireCube(config.CubemapName.c_str(), true);
    
    if (!RenderingSystem::TextureMapAcquireResources(&cubemap)) {
        MFATAL("Невозможно получить ресурсы для текстуры кубической карты.");
        return false;
    }

    g = GeometrySystem::Acquire(config.GConfig, true);
    RenderFrameNumber= INVALID::U64ID;

    auto SkyboxShader = ShaderSystem::GetShader("Shader.Builtin.Skybox");  // ЗАДАЧА: разрешить настраиваемый шейдер.
    TextureMap* maps[1] = {&cubemap};
    if (!RenderingSystem::ShaderAcquireInstanceResources(SkyboxShader, maps, InstanceID)) {
        MFATAL("Невозможно получить ресурсы шейдера для текстуры скайбокса.");
        return false;
    }

    return true;
}

bool Skybox::Unload()
{
    auto SkyboxShader = ShaderSystem::GetShader("Shader.Builtin.Skybox");  // ЗАДАЧА: разрешить настраиваемый шейдер.
    RenderingSystem::ShaderReleaseInstanceResources(SkyboxShader, InstanceID);
    InstanceID = INVALID::ID;
    RenderingSystem::TextureMapReleaseResources(&cubemap);

    RenderFrameNumber = INVALID::U64ID;

    config.GConfig.Dispose();
    if (config.CubemapName) {
        if (cubemap.texture) {
            TextureSystem::Release(config.CubemapName.c_str());
            cubemap.texture = nullptr;
        }

        // u32 length = MString::Length(config.CubemapName);
        // MemorySystem::Free((void*)config.CubemapName, (length + 1) * sizeof(char), Memory::String);
        config.CubemapName = nullptr;
    }

    GeometrySystem::Release(g);

    return true;
}

void *Skybox::operator new(u64 size)
{
    return MemorySystem::Allocate(size, Memory::Scene);
}

void Skybox::operator delete(void *ptr, u64 size)
{
    MemorySystem::Free(ptr, size, Memory::Scene);
}
