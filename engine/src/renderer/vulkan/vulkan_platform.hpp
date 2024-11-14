#pragma once

#include "defines.hpp"

class VulkanAPI;
template<typename T> class DArray;

bool PlatformCreateVulkanSurface(VulkanAPI *VkAPI);

/*
 * Добавляет имена необходимых расширений для этой платформы 
 * к NameDarray, который должен быть создан и передан.
 */
void PlatformGetRequiredExtensionNames(DArray<const char*>& NameDarray);