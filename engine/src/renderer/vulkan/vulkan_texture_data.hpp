#pragma once

#include "renderer/vulkan/vulkan_image.hpp"

struct VulkanTextureData {
    VulkanImage image;

    constexpr VulkanTextureData() : image() {}
    constexpr VulkanTextureData(VulkanImage image) : image(image) {}
    constexpr VulkanTextureData(const VulkanTextureData& value) : image(value.image) {}

    void* operator new(u64 size) { 
        return MMemory::Allocate(size, MemoryTag::Texture); 
    }
    void operator delete(void* ptr, u64 size) { 
        MMemory::Free(ptr, size, MemoryTag::Texture); 
    }
};