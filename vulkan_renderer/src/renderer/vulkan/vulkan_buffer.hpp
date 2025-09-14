/// @file vulkan_buffer.hpp
/// @author 
/// @brief Буфер данных, специфичный для Vulkan.
/// @version 1.0
/// @date 
/// 
/// @copyright 

#pragma once

#include <vulkan/vulkan.h>
#include "containers/freelist.hpp"
#include "core/memory_system.h"

using VkBufferUsageFlag = u32;

struct VulkanBuffer
{
    VkBuffer handle;                            // Дескриптор внутреннего буфера.
    VkBufferUsageFlag usage;                    // Флаги использования.
    bool IsLocked;                              // Указывает, заблокирована ли в данный момент память буфера.
    VkDeviceMemory memory;                      // Память, используемая буфером.
    VkMemoryRequirements MemoryRequirements;    // Требования к памяти для этого буфера.
    i32 MemoryIndex;                            // Индекс памяти, используемой буфером.
    u32 MemoryPropertyFlags;                    // Флаги свойств для памяти, используемой буфером.

    constexpr VulkanBuffer() : handle(), usage(), IsLocked(), memory(), MemoryRequirements(), MemoryIndex(), MemoryPropertyFlags() {}
    // constexpr VulkanBuffer(const VulkanBuffer& b)
    // : 
    // handle(b.handle), 
    // usage(b.usage), 
    // IsLocked(b.IsLocked), 
    // memory(b.memory), 
    // MemoryRequirements(b.MemoryRequirements), 
    // MemoryIndex(b.MemoryIndex), 
    // MemoryPropertyFlags(b.MemoryPropertyFlags) {}

    void* operator new(u64 size) {
        return MemorySystem::Allocate(size, Memory::Vulkan);
    }
    void operator delete(void* ptr, u64 size) {
        MemorySystem::Free(ptr, size, Memory::Vulkan);
    }
};
