#include "resource_system.h"
#include "memory/linear_allocator.h"

#include "core/memory_system.h"
#include <new>

void RegisterLoaders();

constexpr ResourceSystem::ResourceSystem(ResourceSystemConfig& config, ResourceLoader* RegisteredLoaders) 
: MaxLoaderCount(config.MaxLoaderCount), AssetBasePath(config.AssetBasePath), RegisteredLoaders(RegisteredLoaders) {
    for (u64 i = 0; i < config.MaxLoaderCount; i++) {
        this->RegisteredLoaders[i].id = INVALID::ID;
    }
}

ResourceSystem* ResourceSystem::Initialize(u64& MemoryRequirement, void* memory, ResourceSystemConfig& config)
{
    if (config.MaxLoaderCount == 0) {
        MFATAL("ResourceSystem::Initialize е удалось, поскольку максимальное количество загрузчиков (MaxLoaderCount) = 0.");
        return nullptr;
    }

    MemoryRequirement = sizeof(ResourceSystem) + (sizeof(ResourceLoader) * config.MaxLoaderCount);

    if (!memory) {
        return nullptr;
    }

    if (!state) {
        ResourceLoader* ResourceLoaderPtr = reinterpret_cast<ResourceLoader*> (reinterpret_cast<u8*>(memory) + sizeof(ResourceSystem));
        state = new(memory) ResourceSystem(config, ResourceLoaderPtr);
        RegisterLoaders();
    }

    MINFO("Система ресурсов инициализируется с использованием базового пути '%s'.", state->AssetBasePath);

    return state;
}

bool ResourceSystem::RegisterLoader(eResource::Type type, MString && CustomType, const char *TypePath)
{
    // Если встроеный загрузчик уже зарегистрирован в системе, то мы предупреждаем об этом и возвращаем true
    i32 index = -1;
    if (type < eResource::Custom && state->RegisteredLoaders[type].id != INVALID::ID) {
        MWARN("ResourceSystem::RegisterLoader — загрузчик типа %d уже существует и не будет зарегистрирован.", type);
        return true;
    } else if (type > eResource::Custom) {
        for (u32 i = eResource::Custom; i < state->MaxLoaderCount; ++i) {
            auto& l = state->RegisteredLoaders[i];
            if (l.id != INVALID::ID && CustomType && l.CustomType == CustomType) {
                MWARN("ResourceSystem::RegisterLoader — загрузчик пользовательского типа %s уже существует и не будет зарегистрирован.", CustomType.c_str());
                return true;
            } else {
                index = i;
            }
        }
    } else index = type;

    auto& loader = state->RegisteredLoaders[index];
    if (loader.id == INVALID::ID) {
        loader.type = type;
        if (CustomType) loader.CustomType = static_cast<MString&&>(CustomType);
        loader.TypePath = TypePath;
        loader.id = index;
        MTRACE("Загрузчик зарегистрирован.");
        return true;
    }

    return false;
}

template <typename T>
bool ResourceSystem::Load(const char *name, void *params, T &OutResource)
{
    auto& l = state->RegisteredLoaders[OutResource.type];
    if (OutResource.type != eResource::Type::Custom) {
        if (l.id != INVALID::ID && l.type == OutResource.type) {
            return l.Load(name, params, OutResource);
        }
    }

    OutResource.LoaderID = INVALID::ID;
    MERROR("ResourceSystem::Load — загрузчик для типа %d не найден.", OutResource.type);
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

const char* ResourceSystem::BasePath()
{
    if (state && state->AssetBasePath) {
        return state->AssetBasePath;
    }     
    
    MERROR("ResourceSystem::BasePath вызывается перед инициализацией и возвращает пустую строку.");
    return "";
}

MString ResourceSystem::TypePath(u16 ResourceType)
{
    if (ResourceType != eResource::Custom) {
        // Выбор загрузчика.
        ResourceLoader& loader = state->RegisteredLoaders[ResourceType];
        if (loader.id != INVALID::ID && loader.type == ResourceType) {
            u32 size = loader.TypePath.Length() + MString::Length(state->AssetBasePath) + 3;
            char *CombinedPath = (char*)MemorySystem::Allocate(sizeof(char) * size, Memory::String);
            MString::Format(CombinedPath, "%s/%s/", state->AssetBasePath, loader.TypePath.c_str());
            // ЗАДАЧА: Возвращать временную строку
            return MString(size, CombinedPath);
        }
    }
        
    MERROR("Попытка запроса базового пути ресурса для неопознанного типа. Будет возвращено значение пустой строки.");
    return MString();
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
    // ResourceSystem::RegisterLoader(eResource::Type::SimpleScene, MString(),    "scenes");
    ResourceSystem::RegisterLoader(eResource::Type::Terrain,     MString(),  "terrains");
}

template bool ResourceSystem::Load<TextResource>       (const char *name, void *params, TextResource        &OutResource);
template bool ResourceSystem::Load<BinaryResource>     (const char *name, void *params, BinaryResource      &OutResource);
template bool ResourceSystem::Load<ImageResource>      (const char *name, void *params, ImageResource       &OutResource);
template bool ResourceSystem::Load<MaterialResource>   (const char *name, void *params, MaterialResource    &OutResource);
template bool ResourceSystem::Load<MeshResource>       (const char *name, void *params, MeshResource        &OutResource);
template bool ResourceSystem::Load<ShaderResource>     (const char *name, void *params, ShaderResource      &OutResource);
template bool ResourceSystem::Load<BitmapFontResource> (const char *name, void *params, BitmapFontResource  &OutResource);
template bool ResourceSystem::Load<SystemFontResource> (const char *name, void *params, SystemFontResource  &OutResource);
template bool ResourceSystem::Load<SimpleSceneResource>(const char *name, void *params, SimpleSceneResource &OutResource);
template bool ResourceSystem::Load<TerrainResource>    (const char *name, void *params, TerrainResource     &OutResource);
// 
// template void ResourceSystem::Unload<TextResource>       (TextResource        &resource);
template void ResourceSystem::Unload<BinaryResource>     (BinaryResource      &resource);
template void ResourceSystem::Unload<ImageResource>      (ImageResource       &resource);
template void ResourceSystem::Unload<MaterialResource>   (MaterialResource    &resource);
template void ResourceSystem::Unload<MeshResource>       (MeshResource        &resource);
template void ResourceSystem::Unload<ShaderResource>     (ShaderResource      &resource);
// template void ResourceSystem::Unload<BitmapFontResource> (BitmapFontResource  &resource);
// template void ResourceSystem::Unload<SystemFontResource> (SystemFontResource  &resource);
// template void ResourceSystem::Unload<SimpleSceneResource>(SimpleSceneResource &resource);
// template void ResourceSystem::Unload<TerrainResource>    (TerrainResource     &resource);
