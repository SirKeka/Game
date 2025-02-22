#include "resource_system.hpp"
#include "core/logger.hpp"
#include "memory/linear_allocator.hpp"

#include "core/mmemory.hpp"
#include <new>

void RegisterLoaders();

struct sResourceSystem
{
    u32 MaxLoaderCount;
    const char* AssetBasePath;              // Относительный базовый путь для активов.

    ResourceLoader* RegisteredLoaders;

    constexpr sResourceSystem(ResourceSystemConfig* config, ResourceLoader* RegisteredLoaders) 
    : MaxLoaderCount(config->MaxLoaderCount), AssetBasePath(config->AssetBasePath), RegisteredLoaders(RegisteredLoaders) {
        for (u64 i = 0; i < config->MaxLoaderCount; i++) {
            this->RegisteredLoaders[i].id = INVALID::ID;
        }
    }
};

static sResourceSystem* state;

bool ResourceSystem::Initialize(u64& MemoryRequirement, void* memory, void* config)
{
    auto pConfig = reinterpret_cast<ResourceSystemConfig*>(config);
    if (pConfig->MaxLoaderCount == 0) {
    MFATAL("ResourceSystem::Initialize е удалось, поскольку максимальное количество загрузчиков (MaxLoaderCount) = 0.");
    return false;
    }

    MemoryRequirement = sizeof(sResourceSystem) + (sizeof(ResourceLoader) * pConfig->MaxLoaderCount);

    if (!memory) {
        return true;
    }

    if (!state) {
        ResourceLoader* ResourceLoaderPtr = reinterpret_cast<ResourceLoader*> (reinterpret_cast<u8*>(memory) + sizeof(sResourceSystem));
        state = new(memory) sResourceSystem(pConfig, ResourceLoaderPtr);
        RegisterLoaders();
    }

    MINFO("Система ресурсов инициализируется с использованием базового пути '%s'.", state->AssetBasePath);

    return true;
}

void ResourceSystem::Shutdown()
{
    if (state) {
        // state->~ResourceSystem(); // delete state;
        state = nullptr;
    }
}

bool ResourceSystem::RegisterLoader(eResource::Type type, MString &&CustomType, const char *TypePath)
{
    // Убедитесь, что загрузчики данного типа еще не существуют.
    i32 index = -1;
    for (u32 i = 0; i < state->MaxLoaderCount; ++i) {
        auto& l = state->RegisteredLoaders[i];
        if (l.id != INVALID::ID) {
            if (l.type == type) {
                MERROR("ResourceSystem::RegisterLoader — загрузчик типа %d уже существует и не будет зарегистрирован.", type);
                return false;
            } else if (CustomType && CustomType.Length() > 0 && l.CustomType == CustomType) {
                MERROR("ResourceSystem::RegisterLoader — загрузчик пользовательского типа %s уже существует и не будет зарегистрирован.", CustomType.c_str());
                return false;
            }
        } else if (index == -1) {
            index = i;
        }
        
    }

    if (state->RegisteredLoaders[index].id == INVALID::ID) {
        state->RegisteredLoaders[index].Create(type, static_cast<MString&&>(CustomType), TypePath);
        state->RegisteredLoaders[index].id = index;
        MTRACE("Загрузчик зарегистрирован.");
        return true;
    }

    return false;
}

template <typename T>
bool ResourceSystem::Load(const char *name, eResource::Type type, void *params, T &OutResource)
{
    auto& l = state->RegisteredLoaders[type];
    if (type != eResource::Type::Custom) {
        if (l.id != INVALID::ID && l.type == type) {
            return l.Load(name, params, OutResource);
        }
    }

    OutResource.LoaderID = INVALID::ID;
    MERROR("ResourceSystem::Load — загрузчик для типа %d не найден.", type);
    return false;

}

template<typename T>
bool ResourceSystem::Load(const char *name, const char *CustomType, void* params, T &OutResource)
{
    if (CustomType && MString::Length(CustomType) > 0) {
        // Выбор загрузчика.
        for (u32 i = 0; i < state->MaxLoaderCount; ++i) {
            auto& l = state->RegisteredLoaders[i];
            if (l.id != INVALID::ID && l.type == eResource::Type::Custom && l.CustomType.Comparei(CustomType)) {
                return l.Load(name, params, OutResource);
            }
        }
    }

    OutResource.LoaderID = INVALID::ID;
    MERROR("ResourceSystem::LoadCustom — загрузчик для типа %s не найден.", CustomType);
    return false;
}

template<typename T>
void ResourceSystem::Unload(T &resource)
{
    if (resource.LoaderID != INVALID::ID) {
        auto& l = state->RegisteredLoaders[resource.LoaderID];
        if (l.id != INVALID::ID) {
            l.Unload(resource);
        }
    }
}

const char *ResourceSystem::BasePath()
{
    if (state->AssetBasePath) {
        return state->AssetBasePath;
    }

    MERROR("ResourceSystem::BasePath вызывается перед инициализацией и возвращает пустую строку.");
    return "";
}

void RegisterLoaders()
{
    // ЗАДАЧА: возможно все эти "загрузчики" не нужны в таком случае их нужно будет удалить
    // ПРИМЕЧАНИЕ: Здесь можно автоматически зарегистрировать известные типы загрузчиков.
    ResourceSystem::RegisterLoader(eResource::Type::Text,        MString(),          "");
    ResourceSystem::RegisterLoader(eResource::Type::Binary,      MString(),          "");
    ResourceSystem::RegisterLoader(eResource::Type::Image,       MString(),  "textures");
    ResourceSystem::RegisterLoader(eResource::Type::Material,    MString(), "materials");
    ResourceSystem::RegisterLoader(eResource::Type::Mesh,        MString(),    "models");
    ResourceSystem::RegisterLoader(eResource::Type::Shader,      MString(),   "shaders");
    ResourceSystem::RegisterLoader(eResource::Type::BitmapFont,  MString(),     "fonts");
    ResourceSystem::RegisterLoader(eResource::Type::SystemFont,  MString(),     "fonts");
    ResourceSystem::RegisterLoader(eResource::Type::SimpleScene, MString(),    "scenes");
    ResourceSystem::RegisterLoader(eResource::Type::Terrain,     MString(),  "terrains");
}

template bool ResourceSystem::Load<TextResource>       (const char *name, eResource::Type type, void *params, TextResource        &OutResource);
template bool ResourceSystem::Load<BinaryResource>     (const char *name, eResource::Type type, void *params, BinaryResource      &OutResource);
template bool ResourceSystem::Load<ImageResource>      (const char *name, eResource::Type type, void *params, ImageResource       &OutResource);
template bool ResourceSystem::Load<MaterialResource>   (const char *name, eResource::Type type, void *params, MaterialResource    &OutResource);
template bool ResourceSystem::Load<MeshResource>       (const char *name, eResource::Type type, void *params, MeshResource        &OutResource);
template bool ResourceSystem::Load<ShaderResource>     (const char *name, eResource::Type type, void *params, ShaderResource      &OutResource);
template bool ResourceSystem::Load<BitmapFontResource> (const char *name, eResource::Type type, void *params, BitmapFontResource  &OutResource);
template bool ResourceSystem::Load<SystemFontResource> (const char *name, eResource::Type type, void *params, SystemFontResource  &OutResource);
template bool ResourceSystem::Load<SimpleSceneResource>(const char *name, eResource::Type type, void *params, SimpleSceneResource &OutResource);
template bool ResourceSystem::Load<TerrainResource>    (const char *name, eResource::Type type, void *params, TerrainResource     &OutResource);

template void ResourceSystem::Unload<TextResource>       (TextResource        &resource);
template void ResourceSystem::Unload<BinaryResource>     (BinaryResource      &resource);
template void ResourceSystem::Unload<ImageResource>      (ImageResource       &resource);
template void ResourceSystem::Unload<MaterialResource>   (MaterialResource    &resource);
template void ResourceSystem::Unload<MeshResource>       (MeshResource        &resource);
template void ResourceSystem::Unload<ShaderResource>     (ShaderResource      &resource);
template void ResourceSystem::Unload<BitmapFontResource> (BitmapFontResource  &resource);
template void ResourceSystem::Unload<SystemFontResource> (SystemFontResource  &resource);
template void ResourceSystem::Unload<SimpleSceneResource>(SimpleSceneResource &resource);
template void ResourceSystem::Unload<TerrainResource>    (TerrainResource     &resource);
