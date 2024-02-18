#pragma once

#include "defines.hpp"
#include <containers/darray.hpp>

<<<<<<< Updated upstream:engine/src/renderer/vulkan/vulcan_platform.hpp
/**
=======
class MWindow;
class VulkanAPI;

bool PlatformCreateVulkanSurface(
    MWindow *window, 
    VulkanAPI *VkAPI);

/*
>>>>>>> Stashed changes:engine/src/renderer/vulkan/vulkan_platform.hpp
 * Добавляет имена необходимых расширений для этой платформы 
 * к NameDarray, который должен быть создан и передан.
 */
void PlatformGetRequiredExtensionNames(DArray<const char*>& NameDarray);