#pragma once
#include "resource_loader.hpp"

class BinaryLoader : public ResourceLoader
{
private:
    /* u32 id;                 *
     * ResourceType type;      *
     * const char* CustomType; *
     * const char* TypePath;   */
public:
    BinaryLoader();
    //~BinaryLoader() {};
private:
    bool Load(const char* name, struct Resource& OutResource) override;
    void Unload(struct Resource& resource) override;
};
