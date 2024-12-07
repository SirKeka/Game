#pragma once
#include <renderer/renderer_types.hpp>

/// @brief Создает новый плагин рендеринга указанного типа.
/// @return указатель на плагин рендринга указанного типа.
MAPI RendererPlugin* VulkanRenderPluginCreate();

/// @brief 
/// @return 
MAPI void VulkanRenderPluginDestroy(RendererPlugin* plugin);