/// @file shader_loader.hpp
/// @author 
/// @brief A resource loader that handles shader config resources.
/// @version 1.0
/// @date 

/// @copyright 

#pragma once

#include "resource_loader.hpp"

class ShaderLoader : public ResourceLoader
{
private:
    /* data */
public:
    ShaderLoader();
    // ~ShaderLoader();
private:
    bool Load(const char* name, struct Resource* OutResource) override;
    void Unload(struct Resource* resource) override;
};
