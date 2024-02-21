#include "vulkan_swapchain.hpp"

#include "core/logger.hpp"
#include "core/mmemory.hpp"
#include "vulkan_device.hpp"
//#include "vulkan_image.hpp"

void create(VulkanAPI* VkAPI, u32 width, u32 height, VulkanSwapchain* swapchain);
void destroy(VulkanAPI* VkAPI, VulkanSwapchain* swapchain);

void VulkanSwapchainCreate(VulkanAPI *VkAPI, u32 width, u32 height, VulkanSwapchain *OutSwapchain)
{
    // Simply create a new one.
    create(VkAPI, width, height, OutSwapchain);
}

void VulkanSwapchainRecreate(VulkanAPI *VkAPI, u32 width, u32 height, VulkanSwapchain *swapchain)
{
    // Уничтожте старую и создайте новую.
    destroy(VkAPI, swapchain);
    create(VkAPI, width, height, swapchain);
}

void VulkanSwapchainDestroy(VulkanAPI *VkAPI, VulkanSwapchain *swapchain)
{
    destroy(VkAPI, swapchain);
}

bool VulkanSwapchainAcquireNextImageIndex(
    VulkanAPI *VkAPI, 
    VulkanSwapchain *swapchain, 
    u64 TimeoutNs, 
    VkSemaphore ImageAvailableSemaphore, 
    VkFence fence, 
    u32 *OutImageIndex)
{
    VkResult result = vkAcquireNextImageKHR(
        VkAPI->Device.LogicalDevice,
        swapchain->handle,
        TimeoutNs,
        ImageAvailableSemaphore,
        fence,
        OutImageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        // Запустите восстановление цепочки подкачки, затем загрузитесь из цикла рендеринга.
        VulkanSwapchainRecreate(VkAPI, VkAPI->FramebufferWidth, VkAPI->FramebufferHeight, swapchain);
        return false;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        MFATAL("Не удалось получить изображение swapchain!");
        return false;
    }

    return true;
}

void VulkanSwapchainPresent(
    VulkanAPI *VkAPI, 
    VulkanSwapchain *swapchain, 
    VkQueue GraphicsQueue, 
    VkQueue PresentQueue, 
    VkSemaphore RenderCompleteSemaphore, 
    u32 PresentImageIndex)
{
    // Верните изображение в swapchain для презентации.
    VkPresentInfoKHR PresentInfo = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    PresentInfo.waitSemaphoreCount = 1;
    PresentInfo.pWaitSemaphores = &RenderCompleteSemaphore;
    PresentInfo.swapchainCount = 1;
    PresentInfo.pSwapchains = &swapchain->handle;
    PresentInfo.pImageIndices = &PresentImageIndex;
    PresentInfo.pResults = 0;

    VkResult result = vkQueuePresentKHR(PresentQueue, &PresentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // Swapchain устарел, неоптимален или произошло изменение размера фреймбуфера. Запустите восстановление swapchain.
        VulkanSwapchainRecreate(VkAPI, VkAPI->FramebufferWidth, VkAPI->FramebufferHeight, swapchain);
    } else if (result != VK_SUCCESS) {
        MFATAL("Не удалось представить изображение цепочки подкачки!");
    }
}

void create(VulkanAPI *VkAPI, u32 width, u32 height, VulkanSwapchain *swapchain)
{
    VkExtent2D SwapchainExtent = {width, height};
    swapchain->MaxFramesInFlight = 2;

    // Choose a swap surface format.
    b8 found = false;
    for (u32 i = 0; i < VkAPI->Device.SwapchainSupport.FormatCount; ++i) {
        VkSurfaceFormatKHR format = VkAPI->Device.SwapchainSupport.formats[i];
        // Preferred formats
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            swapchain->ImageFormat = format;
            found = true;
            break;
        }
    }

    if (!found) {
        swapchain->ImageFormat = VkAPI->Device.SwapchainSupport.formats[0];
    }


    VkPresentModeKHR PresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (u32 i = 0; i < VkAPI->Device.SwapchainSupport.PresentModeCount; ++i) {
        VkPresentModeKHR mode = VkAPI->Device.SwapchainSupport.PresentModes[i];
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            PresentMode = mode;
            break;
        }
    }

    // Запросить поддержку цепочки обмена.
    VulkanDeviceQuerySwapchainSupport(
        VkAPI->Device.PhysicalDevice,
        VkAPI->surface,
        &VkAPI->Device.SwapchainSupport);

    // Объем цепочки обмена
    if (VkAPI->Device.SwapchainSupport.capabilities.currentExtent.width != UINT32_MAX) {
        SwapchainExtent = VkAPI->Device.SwapchainSupport.capabilities.currentExtent;
    }

    // Ограничьте значение, разрешенное графическим процессором.
    VkExtent2D min = VkAPI->Device.SwapchainSupport.capabilities.minImageExtent;
    VkExtent2D max = VkAPI->Device.SwapchainSupport.capabilities.maxImageExtent;
    SwapchainExtent.width = MCLAMP(SwapchainExtent.width, min.width, max.width);
    SwapchainExtent.height = MCLAMP(SwapchainExtent.height, min.height, max.height);

    u32 image_count = VkAPI->Device.SwapchainSupport.capabilities.minImageCount + 1;
    if (VkAPI->Device.SwapchainSupport.capabilities.maxImageCount > 0 && image_count > VkAPI->Device.SwapchainSupport.capabilities.maxImageCount) {
        image_count = VkAPI->Device.SwapchainSupport.capabilities.maxImageCount;
    }

    // Информация о создании Swapchain
    VkSwapchainCreateInfoKHR SwapchainCreateInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    SwapchainCreateInfo.surface = VkAPI->surface;
    SwapchainCreateInfo.minImageCount = image_count;
    SwapchainCreateInfo.imageFormat = swapchain->ImageFormat.format;
    SwapchainCreateInfo.imageColorSpace = swapchain->ImageFormat.colorSpace;
    SwapchainCreateInfo.imageExtent = SwapchainExtent;
    SwapchainCreateInfo.imageArrayLayers = 1;
    SwapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // Setup the queue family indices
    if (VkAPI->Device.GraphicsQueueIndex != VkAPI->Device.PresentQueueIndex) {
        u32 queueFamilyIndices[] = {
            (u32)VkAPI->Device.GraphicsQueueIndex,
            (u32)VkAPI->Device.PresentQueueIndex};
        SwapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        SwapchainCreateInfo.queueFamilyIndexCount = 2;
        SwapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        SwapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        SwapchainCreateInfo.queueFamilyIndexCount = 0;
        SwapchainCreateInfo.pQueueFamilyIndices = 0;
    }

    SwapchainCreateInfo.preTransform = VkAPI->Device.SwapchainSupport.capabilities.currentTransform;
    SwapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    SwapchainCreateInfo.presentMode = PresentMode;
    SwapchainCreateInfo.clipped = VK_TRUE;
    SwapchainCreateInfo.oldSwapchain = 0;

    VK_CHECK(vkCreateSwapchainKHR(VkAPI->Device.logical_device, &SwapchainCreateInfo, VkAPI->allocator, &swapchain->handle));

    // Start with a zero frame index.
    VkAPI->CurrentFrame = 0;

    // Images
    swapchain->ImageCount = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(VkAPI->Device.logical_device, swapchain->handle, &swapchain->image_count, 0));
    if (!swapchain->images) {
        swapchain->images = MMemory::TAllocate<VkImage>(sizeof(VkImage) * swapchain->ImageCount, MEMORY_TAG_RENDERER);
    }
    if (!swapchain->views) {
        swapchain->views = (VkImageView*)kallocate(sizeof(VkImageView) * swapchain->image_count, MEMORY_TAG_RENDERER);
    }
    VK_CHECK(vkGetSwapchainImagesKHR(VkAPI->Device.logical_device, swapchain->handle, &swapchain->image_count, swapchain->images));

    // Views
    for (u32 i = 0; i < swapchain->image_count; ++i) {
        VkImageViewCreateInfo view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        view_info.image = swapchain->images[i];
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = swapchain->ImageFormat.format;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;

        VK_CHECK(vkCreateImageView(VkAPI->Device.logical_device, &view_info, VkAPI->allocator, &swapchain->views[i]));
    }

    // Depth resources
    if (!vulkan_device_detect_depth_format(&VkAPI->Device)) {
        VkAPI->Device.depth_format = VK_FORMAT_UNDEFINED;
        KFATAL("Failed to find a supported format!");
    }

    // Create depth image and its view.
    vulkan_image_create(
        VkAPI,
        VK_IMAGE_TYPE_2D,
        SwapchainExtent.width,
        SwapchainExtent.height,
        VkAPI->Device.depth_format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        true,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        &swapchain->depth_attachment);

    KINFO("Swapchain created successfully.");
}

void destroy(VulkanAPI *VkAPI, VulkanSwapchain *swapchain)
{
}
