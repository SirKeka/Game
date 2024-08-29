#include "resource_system.hpp"
#include "core/logger.hpp"
#include "memory/linear_allocator.hpp"
#include "resources/loader/image_loader.hpp"
#include "resources/loader/material_loader.hpp"
#include "resources/loader/binary_loader.hpp"
#include "resources/loader/text_loader.hpp"
#include "resources/loader/shader_loader.hpp"
#include "resources/loader/mesh_loader.hpp"

#include "core/mmemory.hpp"
#include <new>

ResourceSystem* ResourceSystem::state = nullptr;

constexpr ResourceSystem::ResourceSystem(u32 MaxLoaderCount, const char* BasePath, ResourceLoader* RegisteredLoaders) 
: MaxLoaderCount(MaxLoaderCount), AssetBasePath(BasePath), RegisteredLoaders(RegisteredLoaders) 
{
    // this->RegisteredLoaders = reinterpret_cast<ResourceLoader*>(this + 1);
    // Аннулировать все загрузчики
    // new (RegisteredLoaders) ResourceLoader[MaxLoaderCount]();

    // ПРИМЕЧАНИЕ: Здесь можно автоматически зарегистрировать известные типы загрузчиков.
    RegisterLoader<TextLoader>(ResourceType::Text, MString(), "");
    RegisterLoader<BinaryLoader>(ResourceType::Binary, MString(), "");
    RegisterLoader<ImageLoader>(ResourceType::Image, MString(), "textures");
    RegisterLoader<MaterialLoader>(ResourceType::Material, MString(), "materials");
    RegisterLoader<ShaderLoader>(ResourceType::Shader, MString(), "shaders");
    RegisterLoader<MeshLoader>(ResourceType::Mesh, MString(), "models");
}

bool ResourceSystem::Initialize(u32 MaxLoaderCount, const char* BasePath)
{
    if (MaxLoaderCount == 0) {
        MFATAL("ResourceSystem::Initialize е удалось, поскольку максимальное количество загрузчиков (MaxLoaderCount) = 0.");
        return false;
    }

    if (!state) {
        u64 ResourceSystemSize = sizeof(ResourceSystem) + (sizeof(ResourceLoader) * MaxLoaderCount);
        void* ResourceSystemPtr = LinearAllocator::Instance().Allocate(ResourceSystemSize); 
        ResourceLoader* ResourceLoaderPtr = reinterpret_cast<ResourceLoader*> (reinterpret_cast<u8*>(ResourceSystemPtr) + sizeof(ResourceSystem));
        state = new(ResourceSystemPtr) ResourceSystem(MaxLoaderCount, BasePath, ResourceLoaderPtr);
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

template<typename T>
bool ResourceSystem::RegisterLoader(ResourceType type, const MString& CustomType, const MString& TypePath)
{
    // Убедитесь, что загрузчики данного типа еще не существуют.
    for (u32 i = 0; i < MaxLoaderCount; ++i) {
        ResourceLoader* l = &RegisteredLoaders[i];
        if (l->type != ResourceType::Invalid) {
            if (l->type == type) {
                MERROR("ResourceSystem::RegisterLoader — загрузчик типа %d уже существует и не будет зарегистрирован.", type);
                return false;
            } else if (CustomType && CustomType.Length() > 0 && l->CustomType.Comparei(CustomType)) {
                MERROR("ResourceSystem::RegisterLoader — загрузчик пользовательского типа %s уже существует и не будет зарегистрирован.", CustomType.c_str());
                return false;
            }
        }
    }
    for (u32 i = 0; i < MaxLoaderCount; ++i) {
        if (RegisteredLoaders[i].type == ResourceType::Invalid) {
            new(RegisteredLoaders + i) T(type, CustomType, TypePath);
            RegisteredLoaders[i].id = i;
            MTRACE("Загрузчик зарегистрирован.");
            return true;
        }
    }

    return false;
}

bool ResourceSystem::Load(const char *name, ResourceType type, void* params, Resource &OutResource)
{
    if (type != ResourceType::Custom) {
        // Выбор загрузчика.
        for (u32 i = 0; i < MaxLoaderCount; ++i) {
            auto& l = RegisteredLoaders[i];
            if (l.id != INVALID::ID && l.type == type) {
                return Load(name, l, params, OutResource);
            }
        }
    }

    OutResource.LoaderID = INVALID::ID;
    MERROR("ResourceSystem::Load — загрузчик для типа %d не найден.", type);
    return false;
}

bool ResourceSystem::Load(const char *name, const char *CustomType, void* params, Resource &OutResource)
{
    if (CustomType && MString::Length(CustomType) > 0) {
        // Выбор загрузчика.
        for (u32 i = 0; i < MaxLoaderCount; ++i) {
            auto& l = RegisteredLoaders[i];
            if (l.id != INVALID::ID && l.type == ResourceType::Custom && l.CustomType.Comparei(CustomType)) {
                return Load(name, l, params, OutResource);
            }
        }
    }

    OutResource.LoaderID = INVALID::ID;
    MERROR("ResourceSystem::LoadCustom — загрузчик для типа %s не найден.", CustomType);
    return false;
}

void ResourceSystem::Unload(Resource &resource)
{
    if (resource.LoaderID != INVALID::ID) {
        ResourceLoader* l = &RegisteredLoaders[resource.LoaderID];
        if (l->id != INVALID::ID) {
            l->Unload(resource);
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

bool ResourceSystem::Load(const char *name, ResourceLoader &loader, void* params, Resource &OutResource)
{
    if (!name) {
        OutResource.LoaderID = INVALID::ID;
        return false;
    }

    OutResource.LoaderID = loader.id;
    return loader.Load(name, params, OutResource);
}
/*
void *ResourceSystem::operator new(u64 size)
{
    return LinearAllocator::Instance().Allocate(size + (sizeof(ResourceLoader) * MaxLoaderCount));
}
*/