#pragma once

class VulkanAPI;
template<typename T> class DArray;

bool PlatformCreateVulkanSurface(VulkanAPI *VkAPI);

/// @brief Добавляет имена необходимых расширений для этой платформы к NameDarray, который должен быть создан и передан.
/// @param NameDarray массив имен расширений платфрмы
void PlatformGetRequiredExtensionNames(DArray<const char*>& NameDarray);