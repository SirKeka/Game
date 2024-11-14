#include "resource_system.hpp"
#include "core/logger.hpp"
#include "memory/linear_allocator.hpp"

#include "core/mmemory.hpp"
#include <new>

ResourceSystem* ResourceSystem::state = nullptr;

constexpr ResourceSystem::ResourceSystem(ResourceSystemConfig* config, ResourceLoader* RegisteredLoaders) 
: MaxLoaderCount(config->MaxLoaderCount), AssetBasePath(config->AssetBasePath), RegisteredLoaders(RegisteredLoaders) 
{
    // ПРИМЕЧАНИЕ: Здесь можно автоматически зарегистрировать известные типы загрузчиков.
    RegisterLoader(eResource::Type::Text,       MString(),          "");
    RegisterLoader(eResource::Type::Binary,     MString(),          "");
    RegisterLoader(eResource::Type::Image,      MString(),  "textures");
    RegisterLoader(eResource::Type::Material,   MString(), "materials");
    RegisterLoader(eResource::Type::Shader,     MString(),   "shaders");
    RegisterLoader(eResource::Type::Mesh,       MString(),    "models");
    RegisterLoader(eResource::Type::BitmapFont, MString(),     "fonts");
    RegisterLoader(eResource::Type::SystemFont, MString(),     "fonts");
}

bool ResourceSystem::Initialize(u64& MemoryRequirement, void* memory, void* config)
{
    auto pConfig = reinterpret_cast<ResourceSystemConfig*>(config);
    if (pConfig->MaxLoaderCount == 0) {
    MFATAL("ResourceSystem::Initialize е удалось, поскольку максимальное количество загрузчиков (MaxLoaderCount) = 0.");
    return false;
    }

    MemoryRequirement = sizeof(ResourceSystem) + (sizeof(ResourceLoader) * pConfig->MaxLoaderCount);

    if (!memory) {
        return true;
    }

    if (!state) {
        ResourceLoader* ResourceLoaderPtr = reinterpret_cast<ResourceLoader*> (reinterpret_cast<u8*>(memory) + sizeof(ResourceSystem));
        state = new(memory) ResourceSystem(pConfig, ResourceLoaderPtr);
    }

    MINFO("Система ресурсов инициализируется с использованием базового пути '%s'.", state->AssetBasePath);

    return true;
}

void ResourceSystem::Shutdown()
{
    if (state) {
        state->~ResourceSystem(); // delete state;
        state = nullptr;
    }
}

bool ResourceSystem::RegisterLoader(eResource::Type type, const MString &CustomType, const char *TypePath)
{
    // Убедитесь, что загрузчики данного типа еще не существуют.
    for (u32 i = 0; i < MaxLoaderCount; ++i) {
        auto& l = RegisteredLoaders[i];
        if (l.type != eResource::Type::Invalid) {
            if (l.type == type) {
                MERROR("ResourceSystem::RegisterLoader — загрузчик типа %d уже существует и не будет зарегистрирован.", type);
                return false;
            } else if (CustomType && CustomType.Length() > 0 && l.CustomType == CustomType) {
                MERROR("ResourceSystem::RegisterLoader — загрузчик пользовательского типа %s уже существует и не будет зарегистрирован.", CustomType.c_str());
                return false;
            }
        }
    }
    for (u32 i = 0; i < MaxLoaderCount; ++i) {
        if (RegisteredLoaders[i].type == eResource::Type::Invalid) {
            RegisteredLoaders[i].Create(type, CustomType, TypePath);
            RegisteredLoaders[i].id = i;
            MTRACE("Загрузчик зарегистрирован.");
            return true;
        }
    }

    return false;
}

template<typename T>
bool ResourceSystem::Load(const char *name, const char *CustomType, void* params, T &OutResource)
{
    if (CustomType && MString::Length(CustomType) > 0) {
        // Выбор загрузчика.
        for (u32 i = 0; i < MaxLoaderCount; ++i) {
            auto& l = RegisteredLoaders[i];
            if (l.id != INVALID::ID && l.type == eResource::Type::Custom && l.CustomType.Comparei(CustomType)) {
                return l.Load(name, params, OutResource);
            }
        }
    }

    OutResource.LoaderID = INVALID::ID;
    MERROR("ResourceSystem::LoadCustom — загрузчик для типа %s не найден.", CustomType);
    return false;
}

const char *ResourceSystem::BasePath()
{
    if (state->AssetBasePath) {
        return state->AssetBasePath;
    }

    MERROR("ResourceSystem::BasePath вызывается перед инициализацией и возвращает пустую строку.");
    return "";
}

/*bool ResourceSystem::Load(const char *name, ResourceLoader &loader, void* params, Resource &OutResource)
{
    if (!name) {
        OutResource.LoaderID = INVALID::ID;
        return false;
    }

    OutResource.LoaderID = loader.id;
    return loader.Load(name, params, OutResource);
}*/
