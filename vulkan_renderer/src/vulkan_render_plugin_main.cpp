#include "vulkan_render_plugin_main.hpp"
#include "renderer/vulkan/vulkan_api.hpp"

RendererPlugin *PluginCreate()
{
    return new VulkanAPI();
}

void PluginDestroy(RendererPlugin* plugin)
{
    delete plugin;
}
