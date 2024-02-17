#pragma once

#include "defines.hpp"
#include <containers/darray.hpp>

struct PlatformState;
struct VulkanContext;

bool PlatformCreateVulkanSurface(
    struct PlatformState* PlatState,
    struct VulkanContext* context);

/*
 * Добавляет имена необходимых расширений для этой платформы 
 * к NameDarray, который должен быть создан и передан.      */
void PlatformGetRequiredExtensionNames(DArray<const char*>& NameDarray);