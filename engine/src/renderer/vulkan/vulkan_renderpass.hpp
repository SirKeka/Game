/*
 * Прежде чем завершить создание графического конвейера нужно сообщить Vulkan, 
 * какие буферы (attachments) будут использоваться во время рендеринга. 
 * Необходимо указать, сколько будет буферов цвета, буферов глубины и сэмплов 
 * для каждого буфера. Также нужно указать, как должно обрабатываться содержимое 
 * буферов во время рендеринга. Вся эта информация обернута в объект прохода рендера (render pass).
*/
// ЗАДАЧА: Перенести все это в фаил vulkan_api.cpp?
#pragma once

#include "defines.hpp"
#include "vulkan_command_buffer.hpp"
#include "math/vector4d.hpp"

class VulkanAPI;
class VulkanCommandBuffer;

enum class VulkanRenderpassState 
{
    Ready,
    Recording,
    InRenderPass,
    RecordingEnded,
    Submitted,
    NotAllocated
};

struct VulkanRenderpass
{
    VkRenderPass handle;         // Внутренний дескриптор прохода рендеринга.

    f32 depth;                   // Значение очистки глубины.
    u32 stencil;                 // Значение очистки трафарета.
    bool HasPrevPass;            // Указывает, есть ли предыдущий проход рендеринга.
    bool HasNextPass;            // Указывает, есть ли следующий проход рендеринга.

    VulkanRenderpassState state; // Указывает состояние прохода рендеринга.

    constexpr VulkanRenderpass() : handle(), depth(), stencil(), HasPrevPass(), HasNextPass(), state() {}
    constexpr VulkanRenderpass(f32 depth, u32 stencil, bool HasPrevPass, bool HasNextPass, VulkanAPI* VkAPI);
    
    ~VulkanRenderpass() = default;

    void Destroy(VulkanAPI* VkAPI);

    void* operator new(u64 size) {
        return MMemory::Allocate(size, MemoryTag::Renderer);
    }
    void operator delete(void* ptr, u64 size) {
        MMemory::Free(ptr, size, MemoryTag::Renderer);
    }
};


