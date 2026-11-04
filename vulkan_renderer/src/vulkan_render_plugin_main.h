#pragma once
#include "defines.h"

class RendererPlugin;

/// @brief Создает новый плагин рендеринга указанного типа.
/// @return указатель на плагин рендринга указанного типа.
extern "C" MAPI RendererPlugin* PluginCreate(void* memory, u64* size);

/// @brief 
/// @return 
extern "C" MAPI void PluginDestroy(RendererPlugin* plugin);
