#pragma once
#include "defines.hpp"

enum class ResourceType {
    Text,
    Binary,
    Image,
    Material,
    Mesh,
    Custom
};

class ResourceLoader
{
private:
    u32 id;
    ResourceType type;
    const char* CustomType;
    const char* TypePath;
public:
    ResourceLoader(/* args */);
    virtual ~ResourceLoader() = default;

    virtual bool Load(const char* name, struct Resource* OutResource) = 0;
    virtual void Unload(struct Resource* resource) = 0;
};
