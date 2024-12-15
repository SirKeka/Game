#pragma once
#include <renderer/renderer_types.hpp>

/// @brief Создает новый плагин рендеринга указанного типа.
/// @return указатель на плагин рендринга указанного типа.
extern "C" MAPI RendererPlugin* PluginCreate();

/// @brief 
/// @return 
<<<<<<< Updated upstream
MAPI void PluginDestroy(RendererPlugin* plugin);
=======
extern "C" MAPI void PluginDestroy(RendererPlugin* plugin);
>>>>>>> Stashed changes
