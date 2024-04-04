#pragma once
#include "defines.hpp"
#include "resources/loader/resource_loader.hpp"

struct Resource {
    u32 LoaderID;
    const char* name;
    char* FullPath;
    u64 DataSize;
    void* data;
};

class ResourceSystem
{
private:
    static u32 MaxLoaderCount;
    // Относительный базовый путь для активов.
    char* AssetBasePath;

    class ResourceLoader* RegisteredLoaders;

    static ResourceSystem* state;
    ResourceSystem();
public:
    ~ResourceSystem();
    ResourceSystem(const ResourceSystem&) = delete;
    ResourceSystem& operator= (const ResourceSystem&) = delete;

    bool Initialize();
    void Shutdown();

    bool RegisterLoader(ResourceLoader loader);
    bool Load(const char* name, ResourceType type, Resource* OutResource);
    bool Load(const char* name, const char* CustomType, Resource* OutResource);
    void Unload(Resource* resource);
    const char* BasePath();

    static void SetMaxLoaderCount(u32 value);
    static ResourceSystem* Instance() {return state;}

public:
    void* operator new(u64 size);
};
