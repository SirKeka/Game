#include <entry.hpp>

#include "platform/platform.hpp"

using PFN_PluginCreate = RendererPlugin*(*)();
using PFN_CreateGame = Application*(*)(const ApplicationConfig&);

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

    if (!PlatformDynamicLibraryLoad("vulkan_renderer", config.RendererLibrary)) {
        return false;
    }

    if (!PlatformDynamicLibraryLoadFunction("PluginCreate", config.RendererLibrary)) {
        return false;
    } 
    auto PlaginCreate = reinterpret_cast<PFN_PluginCreate>(config.RendererLibrary.functions[0].pfn);
    config.RenderPlugin = PlaginCreate();

    if (!PlatformDynamicLibraryLoad("testbed_lib", config.GameLibrary)) {
        return false;
    }

    if (!PlatformDynamicLibraryLoadFunction("CreateGame", config.GameLibrary)) {
        return false;
    } 

    auto CreateGame = reinterpret_cast<PFN_CreateGame>(config.GameLibrary.functions[0].pfn);
    OutApplication = CreateGame(config);

    return true;
}