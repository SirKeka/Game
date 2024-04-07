#include "resource_system.hpp"
#include "core/logger.hpp"
#include "memory/linear_allocator.hpp"
#include "resources/loader/image_loader.hpp"
#include "resources/loader/material_loader.hpp"
#include "resources/loader/binary_loader.hpp"
#include "resources/loader/text_loader.hpp"

#include "core/mmemory.hpp"
#include <new>

u32 ResourceSystem::MaxLoaderCount = 0;
ResourceSystem* ResourceSystem::state = nullptr;

ResourceSystem::ResourceSystem(const char* BasePath)  : AssetBasePath(BasePath), RegisteredLoaders(nullptr) 
{
    //MMemory::CopyMem(AssetBasePath, BasePath, sizeof(BasePath));
    this->RegisteredLoaders = reinterpret_cast<ResourceLoader*>(this + sizeof(ResourceSystem));
    // Аннулировать все загрузчики
    new (reinterpret_cast<void*>(RegisteredLoaders)) ResourceLoader[MaxLoaderCount]();
    /*for (u32 i = 0; i < MaxLoaderCount; ++i) {
        RegisteredLoaders[i].id = INVALID_ID;
    }*/

    // ПРИМЕЧАНИЕ: Здесь можно автоматически зарегистрировать известные типы загрузчиков.
    RegisterLoader<TextLoader>(TextLoader());
    RegisterLoader<BinaryLoader>(BinaryLoader());
    RegisterLoader<ImageLoader>(ImageLoader());
    RegisterLoader<MaterialLoader>(MaterialLoader());
}

bool ResourceSystem::Initialize(const char* BasePath)
{
    if (MaxLoaderCount == 0) {
        MFATAL("ResourceSystem::Initialize е удалось, поскольку максимальное количество загрузчиков (MaxLoaderCount) = 0.");
        return false;
    }

    if (!state) {
        state = new ResourceSystem(BasePath);
    }

    MINFO("Система ресурсов инициализируется с использованием базового пути '%s'.", state->AssetBasePath);

    return true;
}

template<typename T>
bool ResourceSystem::RegisterLoader(ResourceLoader loader)
{
    // Убедитесь, что загрузчики данного типа еще не существуют.
    for (u32 i = 0; i < MaxLoaderCount; ++i) {
        ResourceLoader* l = &RegisteredLoaders[i];
        if (l->id != INVALID_ID) {
            if (l->type == loader.type) {
                MERROR("ResourceSystem::RegisterLoader — загрузчик типа %d уже существует и не будет зарегистрирован.", loader.type);
                return false;
            } else if (loader.CustomType && MString::Length(loader.CustomType) > 0 && StringsEquali(l->CustomType, loader.CustomType)) {
                MERROR("ResourceSystem::RegisterLoader — загрузчик пользовательского типа %s уже существует и не будет зарегистрирован.", loader.CustomType);
                return false;
            }
        }
    }
    for (u32 i = 0; i < MaxLoaderCount; ++i) {
        if (RegisteredLoaders[i].id == INVALID_ID) {
            RegisteredLoaders[i].Destroy();
            new(reinterpret_cast<void*>(RegisteredLoaders + i)) T();
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
            ResourceLoader* l = &RegisteredLoaders[i];
            if (l->id != INVALID_ID && l->type == type) {
                return Load(name, l, OutResource);
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
            ResourceLoader* l = &RegisteredLoaders[i];
            if (l->id != INVALID_ID && l->type == ResourceType::Custom && StringsEquali(l->CustomType, CustomType)) {
                return Load(name, l, OutResource);
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
            ResourceLoader* l = &RegisteredLoaders[resource->LoaderID];
            if (l->id != INVALID_ID) {
                l->Unload(resource);
            }
        }
    }
}

const char *ResourceSystem::BasePath()
{
    if (AssetBasePath) {
        return AssetBasePath;
    }

    MERROR("ResourceSystem::BasePath вызывается перед инициализацией и возвращает пустую строку.");
    return "";
}

void ResourceSystem::SetMaxLoaderCount(u32 value)
{
    MaxLoaderCount = value;
}

bool ResourceSystem::Load(const char *name, ResourceLoader *loader, Resource *OutResource)
{
    if (!name || !loader || !OutResource) {
        OutResource->LoaderID = INVALID_ID;
        return false;
    }

    OutResource->LoaderID = loader->id;
    return loader->Load(name, OutResource);
}

void *ResourceSystem::operator new(u64 size)
{
    return LinearAllocator::Instance().Allocate(size + (sizeof(ResourceLoader) * MaxLoaderCount));
}

