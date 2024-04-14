#pragma once
#include "resource_loader.hpp"

class ImageLoader : public ResourceLoader
{
private:
    /* data */
public:
    ImageLoader(); //: id(INVALID_ID), type(), CustomType(), TypePath() {}
    //~ImageLoader() {};
private:
    bool Load(const char* name, struct Resource* OutResource) override;
    void Unload(struct Resource* resource) override;
};
