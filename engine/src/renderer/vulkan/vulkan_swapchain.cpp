#include "vulkan_swapchain.hpp"

#include "vulkan_api.hpp"
//#include "vulkan_device.hpp"
#include "vulkan_image.hpp"
#include "systems/texture_system.hpp"

bool VulkanSwapchain::AcquireNextImageIndex(
    VulkanAPI *VkAPI,  
    u64 TimeoutNs, 
    VkSemaphore ImageAvailableSemaphore, 
    VkFence fence, 
    u32 *OutImageIndex)
{
    VkResult result = vkAcquireNextImageKHR(
        VkAPI->Device.LogicalDevice,
        handle,
        TimeoutNs,
        ImageAvailableSemaphore,
        fence,
        OutImageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        // Запустите восстановление цепочки подкачки, затем загрузитесь из цикла рендеринга.
        Recreate(VkAPI, VkAPI->FramebufferWidth, VkAPI->FramebufferHeight);
        return false;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        MFATAL("Не удалось получить изображение swapchain!");
        return false;
    }

    return true;
}

void VulkanSwapchain::Present(
    VulkanAPI *VkAPI, 
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
    PresentInfo.pSwapchains = &handle;
    PresentInfo.pImageIndices = &PresentImageIndex;
    PresentInfo.pResults = 0;

    VkResult result = vkQueuePresentKHR(PresentQueue, &PresentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // Swapchain устарел, неоптимален или произошло изменение размера фреймбуфера. Запустите восстановление swapchain.
        Recreate(VkAPI, VkAPI->FramebufferWidth, VkAPI->FramebufferHeight);
        MDEBUG("Цепочка обмена воссоздана, поскольку цепочка обмена вернулась устаревшая или неоптимальная.")
    } else if (result != VK_SUCCESS) {
        MFATAL("Не удалось представить изображение цепочки подкачки!");
    }

     // Увеличьте (и зациклите) индекс.
    VkAPI->CurrentFrame = (VkAPI->CurrentFrame + 1) % MaxFramesInFlight;
}

void VulkanSwapchain::Destroy(VulkanAPI *VkAPI)
{   
    vkDeviceWaitIdle(VkAPI->Device.LogicalDevice);
    DepthTexture->Data->Destroy(VkAPI);
    delete DepthTexture->Data;
    DepthTexture->Data = nullptr;

    // ЗАДАЧА: после выхода из функции main() попадаем в фаил exe_comon.inl который перенаправляет снова в этот цикл
    // Уничтожайте только представления, а не изображения, поскольку они принадлежат цепочке обмена и, следовательно, уничтожаются, когда это происходит.
    for (u32 i = 0; i < ImageCount; ++i) {
        VulkanImage& image = *RenderTextures[i]->Data;
        vkDestroyImageView(VkAPI->Device.LogicalDevice, image.view, VkAPI->allocator);
    }

    vkDestroySwapchainKHR(VkAPI->Device.LogicalDevice, handle, VkAPI->allocator);

    for (u32 i = 0; i < ImageCount; i++) {
        delete RenderTextures[i]->Data;
    }
}

VulkanSwapchain::VulkanSwapchain(VulkanAPI *VkAPI, u32 width, u32 height)
{
    Create(VkAPI, width, height);
}

VulkanSwapchain::~VulkanSwapchain()
{
}

void VulkanSwapchain::Create(VulkanAPI *VkAPI, u32 width, u32 height)
{
    VkExtent2D SwapchainExtent = {width, height};

    // Выберите формат поверхности подкачки.
    bool found = false;
    for (u32 i = 0; i < VkAPI->Device.SwapchainSupport.FormatCount; ++i) {
        VkSurfaceFormatKHR format = VkAPI->Device.SwapchainSupport.formats[i];
        // Предпочтительные форматы
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            ImageFormat = format;
            found = true;
            break;
        }
    }

    if (!found) {
        ImageFormat = VkAPI->Device.SwapchainSupport.formats[0];
    }

    // При высокой скорости рендера кадров выбирается самый последний
    VkPresentModeKHR PresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (u32 i = 0; i < VkAPI->Device.SwapchainSupport.PresentModeCount; ++i) {
        VkPresentModeKHR mode = VkAPI->Device.SwapchainSupport.PresentModes[i];
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            PresentMode = mode;
            break;
        }
    }

    // Запросить поддержку цепочки обмена.
    VkAPI->Device.QuerySwapchainSupport(VkAPI->surface);

    // Объем цепочки обмена
    if (VkAPI->Device.SwapchainSupport.capabilities.currentExtent.width != UINT32_MAX) {
        SwapchainExtent = VkAPI->Device.SwapchainSupport.capabilities.currentExtent;
    }

    // Ограничьте значение, разрешенное графическим процессором.
    VkExtent2D min = VkAPI->Device.SwapchainSupport.capabilities.minImageExtent;
    VkExtent2D max = VkAPI->Device.SwapchainSupport.capabilities.maxImageExtent;
    SwapchainExtent.width = MCLAMP(SwapchainExtent.width, min.width, max.width);
    SwapchainExtent.height = MCLAMP(SwapchainExtent.height, min.height, max.height);

    u32 ImageCount = VkAPI->Device.SwapchainSupport.capabilities.minImageCount + 1;
    if (VkAPI->Device.SwapchainSupport.capabilities.maxImageCount > 0 && ImageCount > VkAPI->Device.SwapchainSupport.capabilities.maxImageCount) {
        ImageCount = VkAPI->Device.SwapchainSupport.capabilities.maxImageCount;
    }

    MaxFramesInFlight = ImageCount - 1;  // Тройная буферизация

    // Информация о создании Swapchain
    VkSwapchainCreateInfoKHR SwapchainCreateInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    SwapchainCreateInfo.surface = VkAPI->surface;
    SwapchainCreateInfo.minImageCount = ImageCount;
    SwapchainCreateInfo.imageFormat = ImageFormat.format;
    SwapchainCreateInfo.imageColorSpace = ImageFormat.colorSpace;
    SwapchainCreateInfo.imageExtent = SwapchainExtent;
    SwapchainCreateInfo.imageArrayLayers = 1;
    SwapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // Настройте индексы семейства очередей.
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

    VK_CHECK(vkCreateSwapchainKHR(VkAPI->Device.LogicalDevice, &SwapchainCreateInfo, VkAPI->allocator, &handle));

    // Начните с нулевого индекса кадра.
    VkAPI->CurrentFrame = 0;

    // Изображения
    //ImageCount = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(VkAPI->Device.LogicalDevice, handle, &this->ImageCount, 0));

    if (!RenderTextures) {
        RenderTextures = new Texture*[this->ImageCount]; //(Texture**)kallocate(sizeof(Texture*) * ImageCount, MEMORY_TAG_RENDERER);
        // При создании массива внутренние объекты текстуры также еще не созданы.
        for (u32 i = 0; i < this->ImageCount; ++i) {
            VulkanImage* data = new VulkanImage();

            char TexName[38] = "__internal_vulkan_swapchain_image_0__";
            TexName[34] = '0' + (char)i;

            RenderTextures[i] = TextureSystem::Instance()->WrapInternal(
                TexName,
                SwapchainExtent.width,
                SwapchainExtent.height,
                4,
                false,
                true,
                false,
                data);
            if (!RenderTextures[i]) {
                MFATAL("Не удалось создать новую текстуру изображения цепочки обмена!");
                return;
            }
        }
    } else {
        for (u32 i = 0; i < this->ImageCount; ++i) {
            // Просто обновите размеры.
            TextureSystem::Instance()->Resize(RenderTextures[i], SwapchainExtent.width, SwapchainExtent.height, false);
        }
    }
    VkImage SwapchainImages[32];
    VK_CHECK(vkGetSwapchainImagesKHR(VkAPI->Device.LogicalDevice, handle, &this->ImageCount, SwapchainImages));
    for (u32 i = 0; i < this->ImageCount; ++i) {
        // Обновите внутренний образ для каждого.
        VulkanImage& image = *RenderTextures[i]->Data;
        image.handle = SwapchainImages[i];
        image.width = SwapchainExtent.width;
        image.height = SwapchainExtent.height;
    }

    // Views
    for (u32 i = 0; i < this->ImageCount; ++i) {
        VulkanImage& image = *RenderTextures[i]->Data;
        VkImageViewCreateInfo ViewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        ViewInfo.image = image.handle;
        ViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        ViewInfo.format = ImageFormat.format;
        ViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        ViewInfo.subresourceRange.baseMipLevel = 0;
        ViewInfo.subresourceRange.levelCount = 1;
        ViewInfo.subresourceRange.baseArrayLayer = 0;
        ViewInfo.subresourceRange.layerCount = 1;

        VK_CHECK(vkCreateImageView(VkAPI->Device.LogicalDevice, &ViewInfo, VkAPI->allocator, &image.view));
    }

    // Ресурсы глубины
    if (!VkAPI->Device.DetectDepthFormat()) {
        VkAPI->Device.DepthFormat = VK_FORMAT_UNDEFINED;
        MFATAL("Не удалось найти поддерживаемый формат!");
    }

    // Создайте изображение глубины и его вид.
    VulkanImage* image = new VulkanImage(VkAPI,
        VK_IMAGE_TYPE_2D,
        SwapchainExtent.width,
        SwapchainExtent.height,
        VkAPI->Device.DepthFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        true,
        VK_IMAGE_ASPECT_DEPTH_BIT
    );
    
    // Оберните его в текстуру.
    VkAPI->swapchain.DepthTexture = TextureSystem::Instance()->WrapInternal(
        "__moon_default_depth_texture__",
        SwapchainExtent.width,
        SwapchainExtent.height,
        VkAPI->Device.DepthChannelCount,
        false,
        true,
        false,
        image
    );

    MINFO("Swapchain успешно создан.");
}

void VulkanSwapchain::Recreate(VulkanAPI *VkAPI, u32 width, u32 height)
{
    // Уничтожте старую и создайте новую.
    Destroy(VkAPI);
    Create(VkAPI, width, height);
}
