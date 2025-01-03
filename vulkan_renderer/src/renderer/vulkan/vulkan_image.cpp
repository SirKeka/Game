#include "vulkan_image.hpp"
#include "vulkan_api.hpp"
#include "vulkan_command_buffer.hpp"
#include "core/asserts.hpp"

VulkanImage::VulkanImage(const Config &config)
{
    Create(config);
}

VulkanImage::~VulkanImage()
{
    /*handle = 0;
    memory = 0;
    view = 0;
    width = 0;
    height = 0;*/
}

void VulkanImage::Create(const Config &config)
{
    // Копировать параметры
    this->MemoryFlags = config.MemoryFlags;
    this->width = config.width;
    this->height = config.height;

    // Информация о создании.
    VkImageCreateInfo ImageCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    switch (config.type) {
        default:
        case TextureType::_2D:
        case TextureType::Cube:  // Тип изображения куба намеренно не предусмотрен.
            ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
            break;
    }
    ImageCreateInfo.extent.width = config.width;
    ImageCreateInfo.extent.height = config.height;
    ImageCreateInfo.extent.depth = 1;                                          // ЗАДАЧА: Поддержка настраиваемой глубины.
    ImageCreateInfo.mipLevels = 4;                                             // ЗАДАЧА: Поддержка мип-маппинга
    ImageCreateInfo.arrayLayers = config.type == TextureType::Cube ? 6 : 1;    // ЗАДАЧА: Поддержка количества слоев изображения.
    ImageCreateInfo.format = config.format;
    ImageCreateInfo.tiling = config.tiling;
    ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ImageCreateInfo.usage = config.usage;
    ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;                           // ЗАДАЧА: Настраиваемое количество образцов.
    ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;                   // ЗАДАЧА: Настраиваемый режим обмена.
    if (config.type == TextureType::Cube) {
        ImageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }

    VK_CHECK(vkCreateImage(config.VkAPI->Device.LogicalDevice, &ImageCreateInfo, config.VkAPI->allocator, &handle));

    // Запросить требования к памяти.
    vkGetImageMemoryRequirements(config.VkAPI->Device.LogicalDevice, handle, &MemoryRequirements);

    i32 MemoryType = config.VkAPI->FindMemoryIndex(MemoryRequirements.memoryTypeBits, config.MemoryFlags);
    if (MemoryType == -1) {
        MERROR("Требуемый тип памяти не найден. Изображение недействительно.");
    }

    // Выделить память
    VkMemoryAllocateInfo MemoryAllocateInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    MemoryAllocateInfo.allocationSize = MemoryRequirements.size;
    MemoryAllocateInfo.memoryTypeIndex = MemoryType;
    VK_CHECK(vkAllocateMemory(config.VkAPI->Device.LogicalDevice, &MemoryAllocateInfo, config.VkAPI->allocator, &memory));

    // Свяжите память
    VK_CHECK(vkBindImageMemory(config.VkAPI->Device.LogicalDevice, handle, memory, 0));  // ЗАДАЧА: настраиваемое смещение памяти.

    // Report the memory as in-use.
    bool IsDeviceMemory = (MemoryFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    MemorySystem::AllocateReport(MemoryRequirements.size, IsDeviceMemory ? Memory::GPULocal : Memory::Vulkan);

    // Создать представление
    if (config.CreateView) {
        view = 0;
        ViewCreate(config.VkAPI, config.type, config.format, config.ViewAspectFlags);
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
    VulkanCommandBuffer &CommandBuffer, 
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
    } else if (OldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        // Переход от макета источника передачи к макету только для чтения шейдера.
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        // От этапа копирования к...
        SourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

        // Этап фрагмента.
        DestStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        // Неважно, на какой стадии находится конвейер в начале.
        SourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

        // Используется для копирования
        DestStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else {
        MFATAL("неподдерживаемый переход макета!");
        return;
    }

    vkCmdPipelineBarrier(
        CommandBuffer.handle,
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
    MemorySystem::ZeroMem(&region, sizeof(VkBufferImageCopy));
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

void VulkanImage::CopyToBuffer(VulkanAPI *VkAPI, TextureType type, VkBuffer buffer, VulkanCommandBuffer &CommandBuffer)
{
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = type == TextureType::Cube ? 6 : 1;

    region.imageExtent.width = width;
    region.imageExtent.height = height;
    region.imageExtent.depth = 1;

    vkCmdCopyImageToBuffer(
        CommandBuffer.handle,
        handle,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        buffer,
        1,
        &region);
}

void VulkanImage::CopyPixelToBuffer(VulkanAPI *VkAPI, TextureType type, VkBuffer buffer, u32 x, u32 y, VulkanCommandBuffer &CommandBuffer)
{
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = type == TextureType::Cube ? 6 : 1;

    region.imageOffset.x = x;
    region.imageOffset.y = y;
    region.imageExtent.width = 1;
    region.imageExtent.height = 1;
    region.imageExtent.depth = 1;

    vkCmdCopyImageToBuffer(
        CommandBuffer.handle,
        handle,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        buffer,
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
    // Сообщить, что память больше не используется.
    bool IsDeviceMemory = (MemoryFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    MemorySystem::FreeReport(MemoryRequirements.size, IsDeviceMemory ? Memory::GPULocal : Memory::Vulkan);
    MemorySystem::ZeroMem(&MemoryRequirements, sizeof(VkMemoryRequirements));
}

void *VulkanImage::operator new(u64 size)
{
    return MemorySystem::Allocate(size, Memory::Texture);
}

void VulkanImage::operator delete(void *ptr, u64 size)
{
    MemorySystem::Free(ptr, size, Memory::Texture);
}
