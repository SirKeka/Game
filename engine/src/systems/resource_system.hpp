#pragma once
#include "defines.hpp"
#include "resources/loader/resource_loader.hpp"
#include "core/mmemory.hpp"

/// @brief Общая структура ресурса. Все загрузчики ресурсов загружают в них данные.
struct Resource {
    u32 LoaderID{};             // Идентификатор загрузчика, обрабатывающего этот ресурс.
    MString name;               // Название ресурса.
    MString FullPath;           // Полный путь к файлу ресурса.
    u64 DataSize{};             // Размер данных ресурса в байтах.
    void* data{nullptr};        // Данные ресурса.
    constexpr Resource() : LoaderID(), name(), FullPath(), DataSize(), data(nullptr) {}
};

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

    static bool Initialize(u32 MaxLoaderCount, const char* BasePath);
    static void Shutdown();

    template<typename T>
    /// @brief Регистрирует указанный загрузчик ресурсов в системе.
    /// @param type
    /// @param CustomType пользовательский тип ресурса.
    /// @param TypePath 
    /// @return 
    MAPI bool RegisterLoader(ResourceType type, const MString& CustomType, const MString& TypePath);
    /// @brief Загружает ресурс с указанным именем.
    /// @param name имя ресурса для загрузки.
    /// @param type тип ресурса для загрузки.
    /// @param params параметры, передаваемые загрузчику, или nullptr.
    /// @param OutResource ссылка на недавно загруженный ресурс.
    /// @return true в случае успеха; в противном случае false.
    MAPI bool Load(const char* name, ResourceType type, void* params, Resource& OutResource);
    /// @brief Загружает ресурс с указанным именем и пользовательского типа.
    /// @param name имя ресурса для загрузки.
    /// @param CustomType пользовательский тип ресурса.
    /// @param params параметры, передаваемые загрузчику, или nullptr.
    /// @param OutResource ссылка на недавно загруженный ресурс.
    /// @return true в случае успеха; в противном случае false.
    MAPI bool Load(const char* name, const char* CustomType, void* params, Resource& OutResource);
    /// @brief Выгружает указанный ресурс.
    /// @param resource ссылка на ресурс который нужно выгрузить.
    MAPI void Unload(Resource& resource);
    MAPI const char* BasePath();

    static MINLINE ResourceSystem* Instance() { return state; }
private:
    bool Load(const char* name, ResourceLoader& loader, void* params, Resource& OutResource);
public:
    // void* operator new(u64 size);
};
