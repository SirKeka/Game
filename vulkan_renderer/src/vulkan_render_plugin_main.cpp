#include "vulkan_render_plugin_main.hpp"
#include "renderer/vulkan/vulkan_api.hpp"

RendererPlugin *VulkanRenderPluginCreate()
{
    return new VulkanAPI();
}

void VulkanRenderPluginDestroy(RendererPlugin* plugin)
{
    delete plugin;
}
