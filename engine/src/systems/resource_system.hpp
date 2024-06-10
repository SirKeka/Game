#pragma once
#include "defines.hpp"
#include "resources/loader/resource_loader.hpp"

/// @brief Общая структура ресурса. Все загрузчики ресурсов загружают в них данные.
struct Resource {
    u32 LoaderID{};             // Идентификатор загрузчика, обрабатывающего этот ресурс.
    MString name{};             // Название ресурса.
    MString FullPath{};         // Полный путь к файлу ресурса.
    u64 DataSize{};             // Размер данных ресурса в байтах.
    void* data{nullptr};        // Данные ресурса.
    // constexpr Resource() : LoaderID(), name(), FullPath(), DataSize(), data(nullptr) {}
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
    u32 MaxLoaderCount;
    // Относительный базовый путь для активов.
    const char* AssetBasePath;

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
    bool RegisterLoader(ResourceType type, const MString& CustomType, const MString& TypePath);
    bool Load(const char* name, ResourceType type, Resource& OutResource);
    bool Load(const char* name, const char* CustomType, Resource& OutResource);
    void Unload(Resource& resource);
    const char* BasePath();

    static MINLINE ResourceSystem* Instance() { return state; }
private:
    bool Load(const char* name, ResourceLoader* loader, Resource& OutResource);
public:
    // void* operator new(u64 size);
};
