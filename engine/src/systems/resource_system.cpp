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

bool ResourceSystem::RegisterLoader(eResource::Type type, const MString &CustomType, const char *TypePath)
{
    // Убедитесь, что загрузчики данного типа еще не существуют.
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
        }
    }
    for (u32 i = 0; i < state->MaxLoaderCount; ++i) {
        if (state->RegisteredLoaders[i].id == INVALID::ID) {
            state->RegisteredLoaders[i].Create(type, CustomType, TypePath);
            state->RegisteredLoaders[i].id = i;
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

ResourceLoader &ResourceSystem::GetLoader(u32 index)
{
    return state->RegisteredLoaders[index];
}

void RegisterLoaders()
{
    // ПРИМЕЧАНИЕ: Здесь можно автоматически зарегистрировать известные типы загрузчиков.
    ResourceSystem::RegisterLoader(eResource::Type::Text,       MString(),          "");
    ResourceSystem::RegisterLoader(eResource::Type::Binary,     MString(),          "");
    ResourceSystem::RegisterLoader(eResource::Type::Image,      MString(),  "textures");
    ResourceSystem::RegisterLoader(eResource::Type::Material,   MString(), "materials");
    ResourceSystem::RegisterLoader(eResource::Type::Mesh,       MString(),    "models");
    ResourceSystem::RegisterLoader(eResource::Type::Shader,     MString(),   "shaders");
    ResourceSystem::RegisterLoader(eResource::Type::BitmapFont, MString(),     "fonts");
    ResourceSystem::RegisterLoader(eResource::Type::SystemFont, MString(),     "fonts");
}
