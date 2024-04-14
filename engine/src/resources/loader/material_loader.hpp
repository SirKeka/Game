#pragma once
#include "resource_loader.hpp"

class MaterialLoader : public ResourceLoader
{
private:
    /* data */
public:
    MaterialLoader();
    //~MaterialLoader();
private:
    bool Load(const char* name, struct Resource* OutResource) override;
    void Unload(struct Resource* resource) override;
};
