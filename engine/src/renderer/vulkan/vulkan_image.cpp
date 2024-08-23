#include "vulkan_image.hpp"
#include "vulkan_api.hpp"
#include "vulkan_command_buffer.hpp"
#include "core/asserts.hpp"

VulkanImage::VulkanImage(
    VulkanAPI *VkAPI, 
    TextureType type, 
    u32 width, 
    u32 height, 
    VkFormat format, 
    VkImageTiling tiling, 
    VkImageUsageFlags usage, 
    VkMemoryPropertyFlags MemoryFlags, 
    b32 CreateView, 
    VkImageAspectFlags ViewAspectFlags)
{
    Create(VkAPI, type, width, height, format, tiling, usage, MemoryFlags, CreateView, ViewAspectFlags);
}

VulkanImage::~VulkanImage()
{
    /*handle = 0;
    memory = 0;
    view = 0;
    width = 0;
    height = 0;*/
}

void VulkanImage::Create(VulkanAPI *VkAPI, TextureType type, u32 width, u32 height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags MemoryFlags, b32 CreateView, VkImageAspectFlags ViewAspectFlags)
{
    // Копировать параметры
    this->width = width;
    this->height = height;

    // Информация о создании.
    VkImageCreateInfo ImageCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    switch (type) {
        default:
        case TextureType::_2D:
        case TextureType::Cube:  // Тип изображения куба намеренно не предусмотрен.
            ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
            break;
    }
    ImageCreateInfo.extent.width = width;
    ImageCreateInfo.extent.height = height;
    ImageCreateInfo.extent.depth = 1;                                   // ЗАДАЧА: Поддержка настраиваемой глубины.
    ImageCreateInfo.mipLevels = 4;                                      // ЗАДАЧА: Поддержка мип-маппинга
    ImageCreateInfo.arrayLayers = type == TextureType::Cube ? 6 : 1;    // ЗАДАЧА: Поддержка количества слоев изображения.
    ImageCreateInfo.format = format;
    ImageCreateInfo.tiling = tiling;
    ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ImageCreateInfo.usage = usage;
    ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;          // ЗАДАЧА: Настраиваемое количество образцов.
    ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;  // ЗАДАЧА: Настраиваемый режим обмена.
    if (type == TextureType::Cube) {
        ImageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }

    VK_CHECK(vkCreateImage(VkAPI->Device.LogicalDevice, &ImageCreateInfo, VkAPI->allocator, &this->handle));

    // Запросить требования к памяти.
    VkMemoryRequirements MemoryRequirements;
    vkGetImageMemoryRequirements(VkAPI->Device.LogicalDevice, this->handle, &MemoryRequirements);

    i32 MemoryType = VkAPI->FindMemoryIndex(MemoryRequirements.memoryTypeBits, MemoryFlags);
    if (MemoryType == -1) {
        MERROR("Требуемый тип памяти не найден. Изображение недействительно.");
    }

    // Выделить память
    VkMemoryAllocateInfo MemoryAllocateInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    MemoryAllocateInfo.allocationSize = MemoryRequirements.size;
    MemoryAllocateInfo.memoryTypeIndex = MemoryType;
    VK_CHECK(vkAllocateMemory(VkAPI->Device.LogicalDevice, &MemoryAllocateInfo, VkAPI->allocator, &this->memory));

    // Свяжите память
    VK_CHECK(vkBindImageMemory(VkAPI->Device.LogicalDevice, this->handle, this->memory, 0));  // ЗАДАЧА: настраиваемое смещение памяти.

    // Создать представление
    if (CreateView) {
        this->view = 0;
        ViewCreate(VkAPI, type, format, ViewAspectFlags);
    }
}

void VulkanImage::ViewCreate(VulkanAPI *VkAPI, TextureType type, VkFormat format, VkImageAspectFlags AspectFlags)
{
    VkImageViewCreateInfo ViewCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    ViewCreateInfo.image = handle;
    switch (type) {
        case TextureType::Cube:
            ViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
            break;
        default:
        case TextureType::_2D:
            ViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            break;
    }
    ViewCreateInfo.format = format;
    ViewCreateInfo.subresourceRange.aspectMask = AspectFlags;

    // ЗАДАЧА: Сделать настраиваемым
    ViewCreateInfo.subresourceRange.baseMipLevel = 0;
    ViewCreateInfo.subresourceRange.levelCount = 1;
    ViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    ViewCreateInfo.subresourceRange.layerCount = type == TextureType::Cube ? 6 : 1;

    VK_CHECK(vkCreateImageView(VkAPI->Device.LogicalDevice, &ViewCreateInfo, VkAPI->allocator, &this->view));
}

void VulkanImage::TransitionLayout(
    VulkanAPI *VkAPI, 
    TextureType type,
    VulkanCommandBuffer *CommandBuffer, 
    VkFormat format, 
    VkImageLayout OldLayout, 
    VkImageLayout NewLayout)
{
    VkImageMemoryBarrier barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.oldLayout = OldLayout;
    barrier.newLayout = NewLayout;
    barrier.srcQueueFamilyIndex = VkAPI->Device.GraphicsQueueIndex;
    barrier.dstQueueFamilyIndex = VkAPI->Device.GraphicsQueueIndex;
    barrier.image = this->handle;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = type == TextureType::Cube ? 6 : 1;

    VkPipelineStageFlags SourceStage;
    VkPipelineStageFlags DestStage;

    // Не беспокойтесь о старом макете — переход к оптимальному макету (для базовой реализации).
    if (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        // Не важно, на какой стадии находится конвейер в начале.
        SourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

        // Используется для копирования
        DestStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (OldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        // Переход от макета назначения передачи к макету, доступному только для чтения шейдерами.
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        // От стадии копирования до...
        SourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

        // Стадия фрагмента.
        DestStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        MFATAL("неподдерживаемый переход макета!");
        return;
    }

    vkCmdPipelineBarrier(
        CommandBuffer->handle,
        SourceStage, DestStage,
        0,
        0, 0,
        0, 0,
        1, &barrier);
}

void VulkanImage::CopyFromBuffer(
    VulkanAPI *VkAPI, 
    TextureType type,
    VkBuffer buffer, 
    VulkanCommandBuffer *CommandBuffer)
{
    // Region to copy
    VkBufferImageCopy region;
    MMemory::ZeroMem(&region, sizeof(VkBufferImageCopy));
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = type == TextureType::Cube ? 6 : 1;

    region.imageExtent.width = this->width;
    region.imageExtent.height = this->height;
    region.imageExtent.depth = 1;

    vkCmdCopyBufferToImage(
        CommandBuffer->handle,
        buffer,
        this->handle,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);
}

void VulkanImage::Destroy(VulkanAPI *VkAPI)
{
    if (this->view) {
        vkDestroyImageView(VkAPI->Device.LogicalDevice, this->view, VkAPI->allocator);
        this->view = 0;
    }
    if (this->memory) {
        vkFreeMemory(VkAPI->Device.LogicalDevice, this->memory, VkAPI->allocator);
        this->memory = 0;
    }
    if (this->handle) {
        vkDestroyImage(VkAPI->Device.LogicalDevice, this->handle, VkAPI->allocator);
        this->handle = 0;
    }
}

void *VulkanImage::operator new(u64 size)
{
    return MMemory::Allocate(size, Memory::Texture);
}

void VulkanImage::operator delete(void *ptr, u64 size)
{
    MMemory::Free(ptr, size, Memory::Texture);
}
