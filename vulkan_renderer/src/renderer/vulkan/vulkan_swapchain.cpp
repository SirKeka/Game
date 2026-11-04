#include "vulkan_swapchain.h"

#include "vulkan_api.h"
#include "vulkan_image.h"
#include <systems/texture_system.h>

void VulkanSwapchain::Destroy(VulkanAPI *VkAPI)
{   
    vkDeviceWaitIdle(VkAPI->device.LogicalDevice);
    for (u32 i = 0; i < VkAPI->swapchain.ImageCount; ++i) {
        reinterpret_cast<VulkanImage*>(DepthTextures[i].data)->Destroy(VkAPI);
        //delete DepthTextures->Data;
        DepthTextures[i].data = nullptr;
    }

    // ЗАДАЧА: после выхода из функции main() попадаем в фаил exe_comon.inl который перенаправляет снова в этот цикл
    // Уничтожайте только представления, а не изображения, поскольку они принадлежат цепочке обмена и, следовательно, уничтожаются, когда это происходит.
    for (u32 i = 0; i < ImageCount; ++i) {
        auto& image = *reinterpret_cast<VulkanImage*>(RenderTextures[i].data);
        vkDestroyImageView(VkAPI->device.LogicalDevice, image.view, VkAPI->allocator);
    }

    vkDestroySwapchainKHR(VkAPI->device.LogicalDevice, handle, VkAPI->allocator);

    /*for (u32 i = 0; i < ImageCount; i++) {
        delete RenderTextures[i]->Data;
        RenderTextures[i]->Data = nullptr;
    }*/
}

VulkanSwapchain::VulkanSwapchain(VulkanAPI *VkAPI, u32 width, u32 height, RendererConfigFlags flags)
{
    Create(VkAPI, width, height, flags);
}

VulkanSwapchain::~VulkanSwapchain()
{
}

void VulkanSwapchain::Create(VulkanAPI *VkAPI, u32 width, u32 height, RendererConfigFlags flags)
{
    VkExtent2D SwapchainExtent {width, height};

    // Выберите формат поверхности подкачки.
    bool found = false;
    for (u32 i = 0; i < VkAPI->device.SwapchainSupport.FormatCount; ++i) {
        VkSurfaceFormatKHR format = VkAPI->device.SwapchainSupport.formats[i];
        // Предпочтительные форматы
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            ImageFormat = format;
            found = true;
            break;
        }
    }

    if (!found) {
        ImageFormat = VkAPI->device.SwapchainSupport.formats[0];
    }

    // FIFO и MAILBOX поддерживают vsync, IMMEDIATE — нет.
    // ЗАДАЧА: vsync, похоже, по какой-то причине задерживает обновление игры.
    // Теоретически это должно происходить после обновления и до рендеринга.
    this->flags = flags;
    VkPresentModeKHR PresentMode;
    if (flags & RenderingConfigFlagBits::VsyncEnabledBit) {
        PresentMode = VK_PRESENT_MODE_FIFO_KHR;
        // Пробуйте режим почтового ящика только в том случае, если не включен режим энергосбережения.
        if ((flags & RenderingConfigFlagBits::PowerSavingBit) == 0) {
            for (u32 i = 0; i < VkAPI->device.SwapchainSupport.PresentModeCount; ++i) {
                VkPresentModeKHR mode = VkAPI->device.SwapchainSupport.PresentModes[i];
                if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                    PresentMode = mode;
                    break;
                }
            }
        }
        
    } else {
        PresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    }  

    // Запросить поддержку цепочки обмена. ЗАДАЧА: переделать
    VkAPI->device.QuerySwapchainSupport(VkAPI->device.PhysicalDevice, VkAPI->surface, &VkAPI->device.SwapchainSupport);

    // Объем цепочки обмена
    if (VkAPI->device.SwapchainSupport.capabilities.currentExtent.width != UINT32_MAX) {
        SwapchainExtent = VkAPI->device.SwapchainSupport.capabilities.currentExtent;
    }

    // Ограничьте значение, разрешенное графическим процессором.
    VkExtent2D min = VkAPI->device.SwapchainSupport.capabilities.minImageExtent;
    VkExtent2D max = VkAPI->device.SwapchainSupport.capabilities.maxImageExtent;
    SwapchainExtent.width = MCLAMP(SwapchainExtent.width, min.width, max.width);
    SwapchainExtent.height = MCLAMP(SwapchainExtent.height, min.height, max.height);

    u32 ImageCount = VkAPI->device.SwapchainSupport.capabilities.minImageCount + 1;
    if (VkAPI->device.SwapchainSupport.capabilities.maxImageCount > 0 && ImageCount > VkAPI->device.SwapchainSupport.capabilities.maxImageCount) {
        ImageCount = VkAPI->device.SwapchainSupport.capabilities.maxImageCount;
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
    if (VkAPI->device.GraphicsQueueIndex != VkAPI->device.PresentQueueIndex) {
        u32 queueFamilyIndices[] = {
            (u32)VkAPI->device.GraphicsQueueIndex,
            (u32)VkAPI->device.PresentQueueIndex};
        SwapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        SwapchainCreateInfo.queueFamilyIndexCount = 2;
        SwapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        SwapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        SwapchainCreateInfo.queueFamilyIndexCount = 0;
        SwapchainCreateInfo.pQueueFamilyIndices = 0;
    }

    SwapchainCreateInfo.preTransform = VkAPI->device.SwapchainSupport.capabilities.currentTransform;
    SwapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    SwapchainCreateInfo.presentMode = PresentMode;
    SwapchainCreateInfo.clipped = VK_TRUE;
    SwapchainCreateInfo.oldSwapchain = 0;

    VK_CHECK(vkCreateSwapchainKHR(VkAPI->device.LogicalDevice, &SwapchainCreateInfo, VkAPI->allocator, &handle));

    // Начните с нулевого индекса кадра.
    VkAPI->CurrentFrame = 0;

    // Изображения
    //ImageCount = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(VkAPI->device.LogicalDevice, handle, &this->ImageCount, nullptr));

    if (!RenderTextures) {
        RenderTextures = mem_alloc(Texture, this->ImageCount, Memory::Renderer);
        // При создании массива внутренние объекты текстуры также еще не созданы.
        for (u32 i = 0; i < this->ImageCount; ++i) {
            auto data = new VulkanImage();

            char TexName[38] = "__internal_vulkan_swapchain_image_0__";
            TexName[34] = '0' + (char)i;

            auto& t = RenderTextures[i];

            t.SetName(TexName);
            //config.type = TextureType::_2D;
            t.width = SwapchainExtent.width;
            t.height = SwapchainExtent.height;
            t.ChannelCount = 4;
            t.data = data;
            //config.HasTransparency = false;
            t.flags |= Texture::Flag::IsWriteable; // = true;
            //config.RegisterTexture = false;

            TextureSystem::WrapInternal(
                nullptr,
                &RenderTextures[i]
            );
            if (!RenderTextures[i].data) {
                MFATAL("Не удалось создать новую текстуру изображения цепочки обмена!");
                return;
            }
        }
    } else {
        for (u32 i = 0; i < this->ImageCount; ++i) {
            // Просто обновите размеры.
            TextureSystem::Resize(&RenderTextures[i], SwapchainExtent.width, SwapchainExtent.height, false);
        }
    }
    VkImage SwapchainImages[32];
    VK_CHECK(vkGetSwapchainImagesKHR(VkAPI->device.LogicalDevice, handle, &ImageCount, SwapchainImages));
    for (u32 i = 0; i < this->ImageCount; ++i) {
        // Обновите внутренний образ для каждого.
        auto& image = *reinterpret_cast<VulkanImage*>(RenderTextures[i].data);
        image.handle = SwapchainImages[i];
        image.width = SwapchainExtent.width;
        image.height = SwapchainExtent.height;
    }

    // Views
    for (u32 i = 0; i < ImageCount; ++i) {
        auto& image = *(VulkanImage*)RenderTextures[i].data;
        VkImageViewCreateInfo ViewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        ViewInfo.image = image.handle;
        ViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        ViewInfo.format = ImageFormat.format;
        ViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        ViewInfo.subresourceRange.baseMipLevel = 0;
        ViewInfo.subresourceRange.levelCount = 1;
        ViewInfo.subresourceRange.baseArrayLayer = 0;
        ViewInfo.subresourceRange.layerCount = 1;

        VK_CHECK(vkCreateImageView(VkAPI->device.LogicalDevice, &ViewInfo, VkAPI->allocator, &image.view));
    }

    if (!DepthTextures) {
        DepthTextures = mem_alloc(Texture, ImageCount, Memory::Renderer);
    }

    // Ресурсы глубины
    if (!VkAPI->device.DetectDepthFormat()) {
        VkAPI->device.DepthFormat = VK_FORMAT_UNDEFINED;
        MFATAL("Не удалось найти поддерживаемый формат!");
    }

    // Создать изображения глубины/трафарета и их вид.
    for (u32 i = 0; i < VkAPI->swapchain.ImageCount; ++i) {
        char FormattedName[TEXTURE_NAME_MAX_LENGTH]{};
        MString::Format(FormattedName, "_moon_default_depth_stencil_texture_%u", i);
        VulkanImage::Config config = { VkAPI };
        config.type            = TextureType::_2D;
        config.width           = SwapchainExtent.width;
        config.height          = SwapchainExtent.height;
        config.format          = VkAPI->device.DepthFormat;
        config.tiling          = VK_IMAGE_TILING_OPTIMAL;
        config.usage           = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        config.MemoryFlags     = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        config.CreateView      = true;
        config.ViewAspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
        config.name            = FormattedName;

        auto image = new VulkanImage(config);

        auto& DepthTexture = VkAPI->swapchain.DepthTextures[i];
        DepthTexture.SetName(FormattedName);
        DepthTexture.width = SwapchainExtent.width;
        DepthTexture.height = SwapchainExtent.height;
        DepthTexture.MipLevels = 1;
        DepthTexture.ChannelCount = VkAPI->device.DepthChannelCount;
        DepthTexture.flags |= Texture::Flag::IsWriteable;
        DepthTexture.data = image;
    
        // Оберните его в текстуру.
        TextureSystem::WrapInternal(nullptr, &DepthTexture);
    }

    MINFO("Swapchain успешно создан.");
}

void VulkanSwapchain::Recreate(VulkanAPI *VkAPI, u32 width, u32 height)
{
    // Уничтожте старую и создайте новую.
    Destroy(VkAPI);
    Create(VkAPI, width, height, this->flags);
}
