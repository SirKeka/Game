#include "vulkan_render_plugin_main.h"
#include "renderer/vulkan/vulkan_api.h"

RendererPlugin *PluginCreate(void* memory, u64* size)
{
    if (size) {
        *size = sizeof(VulkanAPI);
    }
    
    if (memory) {
        return new(memory) VulkanAPI();
    } else {
        return new VulkanAPI();
    }
}

void PluginDestroy(RendererPlugin* plugin)
{
    delete plugin;
}
