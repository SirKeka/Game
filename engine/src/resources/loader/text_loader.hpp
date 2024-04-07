#pragma once
#include "resource_loader.hpp"

class TextLoader : public ResourceLoader
{
private:
    /* data */
public:
    TextLoader();
    ~TextLoader() = default;
private:
    bool Load(const char* name, struct Resource* OutResource) override;
    void Unload(struct Resource* resource) override;
};
