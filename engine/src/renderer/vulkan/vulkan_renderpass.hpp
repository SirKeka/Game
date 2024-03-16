/*
 * Прежде чем завершить создание графического конвейера нужно сообщить Vulkan, 
 * какие буферы (attachments) будут использоваться во время рендеринга. 
 * Необходимо указать, сколько будет буферов цвета, буферов глубины и сэмплов 
 * для каждого буфера. Также нужно указать, как должно обрабатываться содержимое 
 * буферов во время рендеринга. Вся эта информация обернута в объект прохода рендера (render pass).
*/
#pragma once

#include "defines.hpp"
#include "vulkan_command_buffer.hpp"

class VulkanAPI;
class VulkanCommandBuffer;

enum VulkanRenderPassState 
{
    READY,
    RECORDING,
    IN_RENDER_PASS,
    RECORDING_ENDED,
    SUBMITTED,
    NOT_ALLOCATED
};

class VulkanRenderPass
{
public:
    VkRenderPass handle;
    f32 x, y, w, h;
    f32 r, g, b, a;

    f32 depth;
    u32 stencil;

    VulkanRenderPassState state;
public:
    VulkanRenderPass() = default;
    VulkanRenderPass(
        VulkanAPI* VkAPI, 
        f32 x, f32 y, f32 w, f32 h,
        f32 r, f32 g, f32 b, f32 a,
        f32 depth,
        u32 stencil);
    ~VulkanRenderPass() = default;

    void Destroy(VulkanAPI* VkAPI);

    void Begin(
        VulkanCommandBuffer* CommandBuffer, 
        VkFramebuffer FrameBuffer);

    void End(VulkanCommandBuffer* CommandBuffer);
};


