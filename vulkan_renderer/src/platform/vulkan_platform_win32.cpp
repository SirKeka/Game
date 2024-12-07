#include <platform/platform.hpp>

// Уровень платформы Windows.
#if MPLATFORM_WINDOWS

#define WIN32_LEAN_AND_MEAN
#include "vulkan_platform.hpp"
#include <Windows.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>


#include "renderer/vulkan/vulkan_api.hpp"

struct Win32HandleInfo {
    HINSTANCE HInstance;
    HWND hwnd;
};

void PlatformGetRequiredExtensionNames(DArray<const char*>& NameDarray)
{
    NameDarray.PushBack("VK_KHR_win32_surface");
}

// Создание поверхности для Vulkan
bool PlatformCreateVulkanSurface(VulkanAPI *VkAPI) {
    u64 size = 0;
    PlatformGetHandleInfo(size, nullptr);
    void *block = MemorySystem::Allocate(size, Memory::Renderer);
    PlatformGetHandleInfo(size, block);

    auto handle = reinterpret_cast<Win32HandleInfo*>(block);

    if (!handle) {
        return false;
    }

    VkWin32SurfaceCreateInfoKHR CreateInfo = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
    CreateInfo.hinstance = handle->HInstance;
    CreateInfo.hwnd = handle->hwnd;

    VkResult result = vkCreateWin32SurfaceKHR(VkAPI->instance, &CreateInfo, VkAPI->allocator, &VkAPI->surface);
    if (result != VK_SUCCESS) {
        MFATAL("Не удалось создать поверхность Вулкана.");
        return false;
    }

    return true;
}

#endif