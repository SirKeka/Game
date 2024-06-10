#pragma once
#include "resource_loader.hpp"

class TextLoader : public ResourceLoader
{
private:
    /* u32 id;                 *
     * ResourceType type;      *
     * const char* CustomType; *
     * const char* TypePath;   */ 
    using ResourceLoader::ResourceLoader;
public:
    // TextLoader();
    //~TextLoader() {};
private:
    bool Load(const char* name, struct Resource& OutResource) override;
    void Unload(struct Resource& resource) override;
};
