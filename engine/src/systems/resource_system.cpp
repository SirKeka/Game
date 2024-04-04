#include "resource_system.hpp"
#include "core/logger.hpp"
#include "resources/loader/image_loader.hpp"

u32 ResourceSystem::MaxLoaderCount = 0;
ResourceSystem* ResourceSystem::state = nullptr;

ResourceSystem::ResourceSystem()  : AssetBasePath(nullptr), RegisteredLoaders(nullptr) 
{
    // Аннулировать все загрузчики
    for (u32 i = 0; i < MaxLoaderCount; ++i) {
        RegisteredLoaders[i].id = INVALID_ID;
    }

    // ПРИМЕЧАНИЕ: Здесь можно автоматически зарегистрировать известные типы загрузчиков.
    RegisterLoader(text_resource_loader_create());
    RegisterLoader(binary_resource_loader_create());
    RegisterLoader(image_resource_loader_create());
    RegisterLoader(material_resource_loader_create());
}

ResourceSystem::Initialize()
{
    if (MaxLoaderCount == 0) {
        MFATAL("ResourceSystem::Initialize е удалось, поскольку максимальное количество загрузчиков (MaxLoaderCount) = 0.");
        return false;
    }

    if (!state) {
        state = new ResourceSystem();
    }

    MINFO("Система ресурсов инициализируется с использованием базового пути '%s'.", AssetBasePath);

    return true;
}

bool ResourceSystem::RegisterLoader(ResourceLoader loader)
{
    // Убедитесь, что загрузчики данного типа еще не существуют.
    for (u32 i = 0; i < MaxLoaderCount; ++i) {
        ResourceLoader* l = &RegisteredLoaders[i];
        if (l->id != INVALID_ID) {
            if (l->type == loader.type) {
                MERROR("ResourceSystem::RegisterLoader — загрузчик типа %d уже существует и не будет зарегистрирован.", loader.type);
                return false;
            } else if (loader.CustomType && string_length(loader.CustomType) > 0 && strings_equali(l->CustomType, loader.CustomType)) {
                MERROR("ResourceSystem::RegisterLoader — загрузчик пользовательского типа %s уже существует и не будет зарегистрирован.", loader.CustomType);
                return false;
            }
        }
    }
    for (u32 i = 0; i < MaxLoaderCount; ++i) {
        if (RegisteredLoaders[i].id == INVALID_ID) {
            RegisteredLoaders[i] = loader;
            RegisteredLoaders[i].id = i;
            MTRACE("Загрузчик зарегистрирован.");
            return true;
        }
    }

    return false;
}

bool ResourceSystem::Load(const char *name, ResourceType type, Resource *OutResource)
{
    if (type != ResourceType::Custom) {
        // Выбор загрузчика.
        for (u32 i = 0; i < MaxLoaderCount; ++i) {
            RegisterLoader* l = &RegisteredLoaders[i];
            if (l->id != INVALID_ID && l->type == type) {
                return load(name, l, OutResource);
            }
        }
    }

    OutResource->LoaderID = INVALID_ID;
    MERROR("ResourceSystem::Load — загрузчик для типа %d не найден.", type);
    return false;
}

bool ResourceSystem::Load(const char *name, const char *CustomType, Resource *OutResource)
{
    if (CustomType && MString::Length(CustomType) > 0) {
        // Выбор загрузчика.
        for (u32 i = 0; i < MaxLoaderCount; ++i) {
            RegisterLoader* l = &RegisteredLoaders[i];
            if (l->id != INVALID_ID && l->type == ResourceType::Custom && strings_equali(l->CustomType, CustomType)) {
                return load(name, l, OutResource);
            }
        }
    }

    OutResource->LoaderID = INVALID_ID;
    MERROR("ResourceSystem::LoadCustom — загрузчик для типа %s не найден.", CustomType);
    return false;
}

void ResourceSystem::Unload(Resource *resource)
{
    if (resource) {
        if (resource->LoaderID != INVALID_ID) {
            RegisterLoader* l = &RegisteredLoaders[resource->LoaderID];
            if (l->id != INVALID_ID && l->unload) {
                l->unload(l, resource);
            }
        }
    }
}

const char *ResourceSystem::BasePath()
{
    if (state_ptr) {
        return state_ptr->config.asset_base_path;
    }

    MERROR("ResourceSystem::BasePath вызывается перед инициализацией и возвращает пустую строку.");
    return "";
}

ResourceSystem::SetMaxLoaderCount(u32 value)
{
    MaxLoaderCount = value;
}

void *ResourceSystem::operator new(u64 size)
{
    return LinearAllocator::Instance().Allocate(size + (sizeof(ResourceLoader) * MaxLoaderCount));
}


