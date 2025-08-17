#pragma once
#include "defines.hpp"

class RendererPlugin;

/// @brief Создает новый плагин рендеринга указанного типа.
/// @return указатель на плагин рендринга указанного типа.
extern "C" MAPI RendererPlugin* PluginCreate();

/// @brief 
/// @return 
extern "C" MAPI void PluginDestroy(RendererPlugin* plugin);
