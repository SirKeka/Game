/*
 * Прежде чем завершить создание графического конвейера нужно сообщить Vulkan, 
 * какие буферы (attachments) будут использоваться во время рендеринга. 
 * Необходимо указать, сколько будет буферов цвета, буферов глубины и сэмплов 
 * для каждого буфера. Также нужно указать, как должно обрабатываться содержимое 
 * буферов во время рендеринга. Вся эта информация обернута в объект прохода рендера (render pass).
*/
#pragma once

#include "vulkan_api.hpp"

void VulkanRenderpassCreate(
    VulkanAPI* VkAPI, 
    VulkanRenderpass* OutRenderpass,
    f32 x, f32 y, f32 w, f32 h,
    f32 r, f32 g, f32 b, f32 a,
    f32 depth,
    u32 stencil);

void VulkanRenderpassDestroy(VulkanAPI* VkAPI, VulkanRenderpass* renderpass);

void VulkanRenderpassBegin(
    VulkanCommandBuffer* CommandBuffer, 
    VulkanRenderpass* renderpass,
    VkFramebuffer FrameBuffer);

void VulkanRenderpassEnd(VulkanCommandBuffer* CommandBuffer, VulkanRenderpass* renderpass);