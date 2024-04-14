#pragma once

#include "defines.hpp"
#include <vulkan/vulkan.h>
class VulkanAPI;

class VulkanBuffer
{
public:
    u64 TotalSize;
    VkBuffer handle;
    VkBufferUsageFlagBits usage;
    bool IsLocked;
    VkDeviceMemory memory;
    i32 MemoryIndex;
    u32 MemoryPropertyFlags;
public:
    VulkanBuffer() : TotalSize(), handle(), usage(), IsLocked(), memory(), MemoryIndex(), MemoryPropertyFlags() {}
    ~VulkanBuffer() {};

    bool Create(
        VulkanAPI* VkAPI,
        u64 size,
        VkBufferUsageFlagBits usage,
        u32 MemoryPropertyFlags,
        bool BindOnCreate);

    void Destroy(VulkanAPI* VkAPI);

    bool Resize(
    VulkanAPI* VkAPI,
    u64 NewSize,
    VkQueue queue,
    VkCommandPool pool);

    void Bind(VulkanAPI* VkAPI, u64 offset);

    void* LockMemory(VulkanAPI* VkAPI, u64 offset, u64 size, u32 flags);
    void UnlockMemory(VulkanAPI* VkAPI);

    void LoadData(VulkanAPI* VkAPI, u64 offset, u64 size, u32 flags, const void* data);

    void CopyTo(
        VulkanAPI* VkAPI,
        VkCommandPool pool,
        VkFence fence,
        VkQueue queue,
        u64 SourceOffset,
        VkBuffer dest,
        u64 DestOffset,
        u64 size);
};
