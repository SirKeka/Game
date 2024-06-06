#pragma once
#include "defines.hpp"
#include "resources/loader/resource_loader.hpp"

/// @brief Общая структура ресурса. Все загрузчики ресурсов загружают в них данные.
struct Resource {
    u32 LoaderID;       // Идентификатор загрузчика, обрабатывающего этот ресурс.
    const char* name;   // Название ресурса.
    MString FullPath;   // Полный путь к файлу ресурса.
    u64 DataSize;       // Размер данных ресурса в байтах.
    void* data;         // Данные ресурса.
    
};

struct ImageResourceData {
    u8 ChannelCount;
    u32 width;
    u32 height;
    u8* pixels;
};

// TODO: Сделать шаблонной
class ResourceSystem
{
private:
    static u32 MaxLoaderCount;
    // Относительный базовый путь для активов.
    const char* AssetBasePath;

    class ResourceLoader* RegisteredLoaders;

    static ResourceSystem* state;
    ResourceSystem(const char* BasePath);
public:
    ~ResourceSystem() = default;
    ResourceSystem(const ResourceSystem&) = delete;
    ResourceSystem& operator= (const ResourceSystem&) = delete;

    bool Initialize(const char* BasePath);
    void Shutdown() {}

    template<typename T>
    bool RegisterLoader(ResourceLoader loader);
    bool Load(const char* name, ResourceType type, Resource& OutResource);
    bool Load(const char* name, const char* CustomType, Resource& OutResource);
    void Unload(Resource& resource);
    const char* BasePath();

    static void SetMaxLoaderCount(u32 value);
    static ResourceSystem* Instance() {return state;}
private:
    bool Load(const char* name, ResourceLoader* loader, Resource& OutResource);
public:
    void* operator new(u64 size);
};
