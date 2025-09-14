#include "vulkan_render_plugin_main.h"
#include "renderer/vulkan/vulkan_api.h"

RendererPlugin *PluginCreate()
{
    return new VulkanAPI();
}

void PluginDestroy(RendererPlugin* plugin)
{
    delete plugin;
}
