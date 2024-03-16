#pragma once

#include "defines.hpp"
#include "renderer/vulkan/vulkan_image.hpp"
#include "containers/mstring.hpp"

class VulkanAPI;

struct VulkanTextureData {
    VulkanImage image;
    VkSampler sampler;
};

class Texture
{
public:
    u32 id;
    u32 width;
    u32 height;
    u8 ChannelCount;
    bool HasTransparency;
    u32 generation;
    VulkanTextureData* Data;
public:
    Texture();
    Texture(MString name, 
        bool AutoRelease, 
        i32 width, 
        i32 height, 
        i32 ChannelCount, 
        const u8* pixels, 
        bool HasTransparency,
        VulkanAPI* VkAPI);
    ~Texture() = default;
    Texture(const Texture& t);
    void Create(MString name, 
        bool AutoRelease, 
        i32 width, 
        i32 height, 
        i32 ChannelCount, 
        const u8* pixels, 
        bool HasTransparency,
        VulkanAPI* VkAPI);
    void Destroy(VulkanAPI* VkAPI);

    void* operator new(u64 size);
    void operator delete(void* ptr);
};
