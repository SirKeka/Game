#pragma once
#include "resource_loader.hpp"

class ImageLoader : public ResourceLoader
{
private:
    /* u32 id;                 *
     * ResourceType type;      *
     * const char* CustomType; *
     * const char* TypePath;   */
public:
    ImageLoader();
    //~ImageLoader() {};
private:
    bool Load(const char* name, struct Resource& OutResource) override;
    void Unload(struct Resource& resource) override;
};
