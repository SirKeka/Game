#include <entry.hpp>
#include "game.hpp"

#include "vulkan_render_plugin_main.hpp"

bool CreateApplication(Application*& OutApplication)
{
    ApplicationConfig config;
    config.StartPosX = 100;
    config.StartPosY = 100;
    config.StartWidth = 1280;
    config.StartHeight = 720;
    config.name = "Moon Engine";
    // config.FontConfig = 
    // config.RenderViews = 
    config.RenderPlugin = VulkanRenderPluginCreate();

    OutApplication = new Game(config);

    return true;
}