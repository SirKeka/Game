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
#include "math/vector4d.hpp"

class VulkanAPI;
class VulkanCommandBuffer;

enum class VulkanRenderPassState 
{
    Ready,
    Recording,
    InRenderPass,
    RecordingEnded,
    Submitted,
    NotAllocated
};

enum class RenderpassClearFlag : u8 {
    NoneFlag = 0x0,
    ColourBufferFlag = 0x1,
    DepthBufferFlag = 0x2,
    StencilBufferFlag = 0x4
};
MINLINE constexpr u8
operator&(RenderpassClearFlag x, RenderpassClearFlag y)
{
    return /*static_cast<RenderpassClearFlag>*/
    (static_cast<u8>(x) & static_cast<u8>(y));
}
MINLINE constexpr u8
operator&(u8 x, RenderpassClearFlag y)
{
    return (x & static_cast<u8>(y));
}
MINLINE constexpr RenderpassClearFlag
operator|(RenderpassClearFlag x, RenderpassClearFlag y)
{
    return static_cast<RenderpassClearFlag>
    (static_cast<u8>(x) | static_cast<u8>(y));
}

enum class BuiltinRenderpass : u8 {
    World = 0x01,
    UI = 0x02
};

class VulkanRenderPass
{
public:
    VkRenderPass handle;
    Vector4D<f32> RenderArea;
    Vector4D<f32> ClearColour;

    f32 depth;
    u32 stencil;
    u8 ClearFlags;
    bool HasPrevPass;
    bool HasNextPass;

    VulkanRenderPassState state;
public:
    VulkanRenderPass() = default;
    VulkanRenderPass(
        VulkanAPI* VkAPI, 
        Vector4D<f32> RenderArea,
        Vector4D<f32> ClearColor,
        f32 depth,
        u32 stencil,
        u8 ClearFlags,
        bool HasPrevPass,
        bool HasNextPass);
    ~VulkanRenderPass() = default;

    void Destroy(VulkanAPI* VkAPI);

    void Begin(
        VulkanCommandBuffer* CommandBuffer, 
        VkFramebuffer FrameBuffer);

    void End(VulkanCommandBuffer* CommandBuffer);
};


