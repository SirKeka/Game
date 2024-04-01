#pragma once

#include "defines.hpp"
#include "renderer/vulkan/vulkan_image.hpp"
#include "containers/mstring.hpp"

#define TEXTURE_NAME_MAX_LENGTH 512

class VulkanAPI;

struct VulkanTextureData {
    VulkanImage image;
    VkSampler sampler;
};

enum class TextureUse {
    Unknown = 0x00,
    MapDiffuse = 0x01
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
    char name[TEXTURE_NAME_MAX_LENGTH];
    VulkanTextureData* Data;
public:
    Texture();
    Texture(
        const char* name, 
        i32 width, 
        i32 height, 
        i32 ChannelCount, 
        const u8* pixels, 
        bool HasTransparency,
        VulkanAPI* VkAPI);
    ~Texture() = default;
    Texture(const Texture& t);
    void Create(
        const char* name, 
        i32 width, 
        i32 height, 
        i32 ChannelCount, 
        const u8* pixels, 
        bool HasTransparency,
        VulkanAPI* VkAPI);
    void Destroy(VulkanAPI* VkAPI);


    explicit operator bool() const;
    void* operator new(u64 size);
    void operator delete(void* ptr);
};
