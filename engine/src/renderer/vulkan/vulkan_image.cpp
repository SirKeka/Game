#include "vulkan_image.hpp"

#include "vulkan_device.hpp"

#include "core/mmemory.hpp"
#include "core/logger.hpp"

void VulkanImageCreate(
    VulkanAPI *VkAPI, 
    VkImageType ImageType, 
    u32 width, 
    u32 height, 
    VkFormat format, 
    VkImageTiling tiling, 
    VkImageUsageFlags usage, 
    VkMemoryPropertyFlags MemoryFlags, 
    b32 CreateView, 
    VkImageAspectFlags ViewAspectFlags, 
    VulkanImage *OutImage)
{
    // Копировать параметры
    OutImage->width = width;
    OutImage->height = height;

    // Информация о создании.
    VkImageCreateInfo ImageCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    ImageCreateInfo.extent.width = width;
    ImageCreateInfo.extent.height = height;
    ImageCreateInfo.extent.depth = 1;  // TODO: Поддержка настраиваемой глубины.
    ImageCreateInfo.mipLevels = 4;     // TODO: Поддержка мип-маппинга
    ImageCreateInfo.arrayLayers = 1;   // TODO: Поддержка количества слоев изображения.
    ImageCreateInfo.format = format;
    ImageCreateInfo.tiling = tiling;
    ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ImageCreateInfo.usage = usage;
    ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;          // TODO: Настраиваемое количество образцов.
    ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;  // TODO: Настраиваемый режим обмена.

    VK_CHECK(vkCreateImage(VkAPI->Device.LogicalDevice, &ImageCreateInfo, VkAPI->allocator, &OutImage->handle));

    // Запросить требования к памяти.
    VkMemoryRequirements MemoryRequirements;
    vkGetImageMemoryRequirements(VkAPI->Device.LogicalDevice, OutImage->handle, &MemoryRequirements);

    i32 MemoryType = VkAPI->FindMemoryIndex(MemoryRequirements.memoryTypeBits, MemoryFlags);
    if (MemoryType == -1) {
        MERROR("Требуемый тип памяти не найден. Изображение недействительно.");
    }

    // Выделить память
    VkMemoryAllocateInfo MemoryAllocateInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    MemoryAllocateInfo.allocationSize = MemoryRequirements.size;
    MemoryAllocateInfo.memoryTypeIndex = MemoryType;
    VK_CHECK(vkAllocateMemory(VkAPI->Device.LogicalDevice, &MemoryAllocateInfo, VkAPI->allocator, &OutImage->memory));

    // Свяжите память
    VK_CHECK(vkBindImageMemory(VkAPI->Device.LogicalDevice, OutImage->handle, OutImage->memory, 0));  // TODO: настраиваемое смещение памяти.

    // Создать представление
    if (CreateView) {
        OutImage->view = 0;
        VulkanImageViewCreate(VkAPI, format, OutImage, ViewAspectFlags);
    }
}

void VulkanImageViewCreate(VulkanAPI *VkAPI, VkFormat format, VulkanImage *image, VkImageAspectFlags AspectFlags)
{
    VkImageViewCreateInfo ViewCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    ViewCreateInfo.image = image->handle;
    ViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;  // TODO: Сделать настраиваемым.
    ViewCreateInfo.format = format;
    ViewCreateInfo.subresourceRange.aspectMask = AspectFlags;

    // TODO: Сделать настраиваемым
    ViewCreateInfo.subresourceRange.baseMipLevel = 0;
    ViewCreateInfo.subresourceRange.levelCount = 1;
    ViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    ViewCreateInfo.subresourceRange.layerCount = 1;

    VK_CHECK(vkCreateImageView(VkAPI->Device.LogicalDevice, &ViewCreateInfo, VkAPI->allocator, &image->view));
}

void VulkanImageDestroy(VulkanAPI *VkAPI, VulkanImage *image)
{
    if (image->view) {
        vkDestroyImageView(VkAPI->Device.LogicalDevice, image->view, VkAPI->allocator);
        image->view = 0;
    }
    if (image->memory) {
        vkFreeMemory(VkAPI->Device.LogicalDevice, image->memory, VkAPI->allocator);
        image->memory = 0;
    }
    if (image->handle) {
        vkDestroyImage(VkAPI->Device.LogicalDevice, image->handle, VkAPI->allocator);
        image->handle = 0;
    }
}
