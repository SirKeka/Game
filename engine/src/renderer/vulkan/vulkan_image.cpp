#include "vulkan_image.hpp"

#include "vulkan_device.hpp"

#include "core/mmemory.hpp"
#include "core/logger.hpp"

VulkanImage::VulkanImage(
    VulkanAPI *VkAPI, 
    VkImageType ImageType, 
    u32 width, 
    u32 height, 
    VkFormat format, 
    VkImageTiling tiling, 
    VkImageUsageFlags usage, 
    VkMemoryPropertyFlags MemoryFlags, 
    b32 CreateView, 
    VkImageAspectFlags ViewAspectFlags)
{
    // Копировать параметры
    this->width = width;
    this->height = height;

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

    VK_CHECK(vkCreateImage(VkAPI->Device->LogicalDevice, &ImageCreateInfo, VkAPI->allocator, &this->handle));

    // Запросить требования к памяти.
    VkMemoryRequirements MemoryRequirements;
    vkGetImageMemoryRequirements(VkAPI->Device->LogicalDevice, this->handle, &MemoryRequirements);

    i32 MemoryType = VkAPI->FindMemoryIndex(MemoryRequirements.memoryTypeBits, MemoryFlags);
    if (MemoryType == -1) {
        MERROR("Требуемый тип памяти не найден. Изображение недействительно.");
    }

    // Выделить память
    VkMemoryAllocateInfo MemoryAllocateInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    MemoryAllocateInfo.allocationSize = MemoryRequirements.size;
    MemoryAllocateInfo.memoryTypeIndex = MemoryType;
    VK_CHECK(vkAllocateMemory(VkAPI->Device->LogicalDevice, &MemoryAllocateInfo, VkAPI->allocator, &this->memory));

    // Свяжите память
    VK_CHECK(vkBindImageMemory(VkAPI->Device->LogicalDevice, this->handle, this->memory, 0));  // TODO: настраиваемое смещение памяти.

    // Создать представление
    if (CreateView) {
        this->view = 0;
        ViewCreate(VkAPI, format, ViewAspectFlags);
    }
}

/*void VulkanImage::Create(
    VulkanAPI *VkAPI,
    VkImageType ImageType,
    u32 width,
    u32 height,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags MemoryFlags,
    b32 CreateView,
    VkImageAspectFlags ViewAspectFlags)
{
    
}*/

void VulkanImage::ViewCreate(VulkanAPI *VkAPI, VkFormat format, VkImageAspectFlags AspectFlags)
{
    VkImageViewCreateInfo ViewCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    ViewCreateInfo.image = this->handle;
    ViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;  // TODO: Сделать настраиваемым.
    ViewCreateInfo.format = format;
    ViewCreateInfo.subresourceRange.aspectMask = AspectFlags;

    // TODO: Сделать настраиваемым
    ViewCreateInfo.subresourceRange.baseMipLevel = 0;
    ViewCreateInfo.subresourceRange.levelCount = 1;
    ViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    ViewCreateInfo.subresourceRange.layerCount = 1;

    VK_CHECK(vkCreateImageView(VkAPI->Device->LogicalDevice, &ViewCreateInfo, VkAPI->allocator, &this->view));
}

void VulkanImage::Destroy(VulkanAPI *VkAPI)
{
    if (this->view) {
        vkDestroyImageView(VkAPI->Device->LogicalDevice, this->view, VkAPI->allocator);
        this->view = 0;
    }
    if (this->memory) {
        vkFreeMemory(VkAPI->Device->LogicalDevice, this->memory, VkAPI->allocator);
        this->memory = 0;
    }
    if (this->handle) {
        vkDestroyImage(VkAPI->Device->LogicalDevice, this->handle, VkAPI->allocator);
        this->handle = 0;
    }
}

void *VulkanImage::operator new(u64 size)
{
    return LinearAllocator::Allocate(size);
}
