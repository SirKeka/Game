#pragma once
#include "defines.hpp"
#include "resources/loader/resource_loader.hpp"
#include "core/mmemory.hpp"

// ЗАДАЧА: переделать
class ResourceSystem
{
private:
    u32 MaxLoaderCount;
    const char* AssetBasePath;              // Относительный базовый путь для активов.

    class ResourceLoader* RegisteredLoaders;

    static ResourceSystem* state;
    constexpr ResourceSystem(u32 MaxLoaderCount, const char* BasePath, ResourceLoader* RegisteredLoaders);
public:
    ~ResourceSystem() = default;
    ResourceSystem(const ResourceSystem&) = delete;
    ResourceSystem& operator= (const ResourceSystem&) = delete;

    static bool Initialize(u32 MaxLoaderCount, const char* BasePath, class LinearAllocator& SystemAllocator);
    static void Shutdown();

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
    /// @param OutResource ссылка на недавно загруженный ресурс.
    /// @return true в случае успеха; в противном случае false.
    template<typename T>
    MAPI static bool Load(const char* name, eResource::Type type, void* params, T& OutResource) {
        if (type != eResource::Type::Custom) {
            // Выбор загрузчика.
            for (u32 i = 0; i < state->MaxLoaderCount; ++i) {
                auto& l = state->RegisteredLoaders[i];
                if (l.id != INVALID::ID && l.type == type) {
                    return l.Load(name, params, OutResource);
                }
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
    MAPI static void Unload(T& resource) {
        if (resource.LoaderID != INVALID::ID) {
            auto& l = state->RegisteredLoaders[resource.LoaderID];
            if (l.id != INVALID::ID) {
                l.Unload(resource);
            }
        }
    }
    MAPI static const char* BasePath();

    static MINLINE ResourceSystem* Instance() { return state; }
/*private:
    bool Load(const char* name, ResourceLoader& loader, void* params, Resource& OutResource);
public:
    // void* operator new(u64 size);*/
};
