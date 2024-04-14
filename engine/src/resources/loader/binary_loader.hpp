#pragma once
#include "resource_loader.hpp"

class BinaryLoader : public ResourceLoader
{
private:
    /* data */
public:
    BinaryLoader();
    //~BinaryLoader() {};
private:
    bool Load(const char* name, struct Resource* OutResource) override;
    void Unload(struct Resource* resource) override;
};
