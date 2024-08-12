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
    /* u32 id;                 *
     * ResourceType type;      *
     * const char* CustomType; *
     * const char* TypePath;   */
    using ResourceLoader::ResourceLoader;
public:
    // ShaderLoader();
    // ~ShaderLoader();
private:
    bool Load(const char* name, void* params, struct Resource& OutResource) override;
    void Unload(struct Resource& resource) override;
};
