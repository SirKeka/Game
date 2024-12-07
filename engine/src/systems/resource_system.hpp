#pragma once
#include "defines.hpp"
#include "resources/loader/resource_loader.hpp"
#include "core/mmemory.hpp"

/// @brief Конфигурация для системы ресурсов
struct ResourceSystemConfig {
    /// @brief Максимальное количество загрузчиков, которые могут быть зарегистрированы в этой системе.
    u32 MaxLoaderCount;
    /// @brief Относительный базовый путь для активов.
    const char* AssetBasePath;
};

// ЗАДАЧА: переделать
namespace ResourceSystem
{
    bool Initialize(u64& MemoryRequirement, void* memory, void* config);
    void Shutdown();

    /// @brief Регистрирует указанный загрузчик ресурсов в системе.
    /// @param type
    /// @param CustomType пользовательский тип ресурса.
    /// @param TypePath 
    /// @return 
    MAPI bool RegisterLoader(eResource::Type type, const MString& CustomType, const char* TypePath);
    /// @brief Загружает ресурс с указанным именем.
    /// @param name имя ресурса для загрузки.
    /// @param type тип ресурса для загрузки.
    /// @param params параметры, передаваемые загрузчику, или nullptr.
    /// @param OutResource указатель на недавно загруженный ресурс.
    /// @return true в случае успеха; в противном случае false.
    template<typename T>
    bool Load(const char *name, eResource::Type type, void *params, T &OutResource)
    {
        auto& l = GetLoader(type);
        if (type != eResource::Type::Custom) {
            if (l.id != INVALID::ID && l.type == type) {
                return l.Load(name, params, OutResource);
            }
        }

        OutResource.LoaderID = INVALID::ID;
        MERROR("ResourceSystem::Load — загрузчик для типа %d не найден.", type);
        return false;

    }

    /// @brief Загружает ресурс с указанным именем и пользовательского типа.
    /// @param name имя ресурса для загрузки.
    /// @param CustomType пользовательский тип ресурса.
    /// @param params параметры, передаваемые загрузчику, или nullptr.
    /// @param OutResource ссылка на недавно загруженный ресурс.
    /// @return true в случае успеха; в противном случае false.
    template<typename T>
    MAPI bool Load(const char* name, const char* CustomType, void* params, T& OutResource);

    /// @brief Выгружает указанный ресурс.
    /// @param resource ссылка на ресурс который нужно выгрузить.
    template<typename T>
    MAPI void Unload(T& resource) {
        if (resource.LoaderID != INVALID::ID) {
            auto& l = GetLoader(resource.LoaderID);
            if (l.id != INVALID::ID) {
                l.Unload(resource);
            }
        }
    }

    MAPI const char* BasePath();

    MAPI ResourceLoader& GetLoader(u32 index);
};

