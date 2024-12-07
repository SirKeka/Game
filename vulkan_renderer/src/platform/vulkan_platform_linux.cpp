#include <platform/platform.hpp>

// Уровень платформы Linux.
#if MPLATFORM_LINUX

#include <xcb/xcb.h>

// For surface creation
#define VK_USE_PLATFORM_XCB_KHR
#include <vulkan/vulkan.h>

#include <containers/darray.h>
#include <core/kmemory.h>
#include <core/logger.h>

#include "renderer/vulkan/vulkan_types.inl"
#include "renderer/vulkan/platform/vulkan_platform.h"


typedef struct linux_handle_info {
    xcb_connection_t* connection;
    xcb_window_t window;
} linux_handle_info;

void PlatformGetRequiredExtensionNames(DArray<const char*>& NameDarray) {
    NameDarray.PushBack("VK_KHR_xcb_surface");  // VK_KHR_xlib_surface?
}

// Создание поверхности для Vulkan
b8 PlatformCreateVulkanSurface(VulkanAPI *VkAPI) {
    u64 size = 0;
    PlatformGetHandleInfo(&size, 0);
    void *block = MemorySystem::Allocate(size, Memory::Renderer);
    PlatformGetHandleInfo(&size, block);

    linux_handle_info* handle = (linux_handle_info*)block;

    VkXcbSurfaceCreateInfoKHR create_info = {VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR};
    create_info.connection = handle->connection;
    create_info.window = handle->window;

    VkResult result = vkCreateXcbSurfaceKHR(
        VkAPI->instance,
        &create_info,
        VkAPI->allocator,
        &VkAPI->surface);
    if (result != VK_SUCCESS) {
        MFATAL("Не удалось создать поверхность Vulkan.");
        return false;
    }

    return true;
}

#endif
