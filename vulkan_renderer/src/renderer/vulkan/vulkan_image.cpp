#include "vulkan_image.h"
#include "vulkan_api.h"
#include "vulkan_utils.h"
#include "vulkan_command_buffer.h"
#include "core/asserts.h"

VulkanImage::VulkanImage(Config &config) : name((MString&&)config.name), MemoryFlags(config.MemoryFlags), format(config.format), 
width(config.width), height(config.height), LayerCount(config.LayerCount), MipLevels(config.MipLevels)
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

void VulkanImage::Create(Config &config)
{
    if (LayerCount < 1) { LayerCount = 1; }
    
    // Информация о создании.
    VkImageCreateInfo ImageCreateInfo {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .format = format,
        .mipLevels = MipLevels,
        .arrayLayers = LayerCount,
        .samples = VK_SAMPLE_COUNT_1_BIT,               // ЗАДАЧА: Настраиваемое количество образцов.
        .tiling = config.tiling,
        .usage = config.usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,       // ЗАДАЧА: Настраиваемый режим обмена.
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    switch (config.type) {
        default:
        case TextureType::_2D:
        case TextureType::Cube:     // Тип изображения куба намеренно не предусмотрен.
        case TextureType::_2DArray: // Намеренно не предусмотрен
            ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
            break;
    }

    ImageCreateInfo.extent.width = width;
    ImageCreateInfo.extent.height = height;
    ImageCreateInfo.extent.depth = 1;                   // ЗАДАЧА: Поддержка настраиваемой глубины.

    if (config.type == TextureType::Cube) {
        ImageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }

    VK_CHECK(vkCreateImage(config.VkAPI->device.LogicalDevice, &ImageCreateInfo, config.VkAPI->allocator, &handle));
    VK_SET_DEBUG_OBJECT_NAME(config.VkAPI, VK_OBJECT_TYPE_IMAGE, handle, name.c_str());

    // Запросить требования к памяти.
    vkGetImageMemoryRequirements(config.VkAPI->device.LogicalDevice, handle, &MemoryRequirements);

    i32 MemoryType = config.VkAPI->FindMemoryIndex(MemoryRequirements.memoryTypeBits, config.MemoryFlags);
    if (MemoryType == -1) {
        MERROR("Требуемый тип памяти не найден. Изображение недействительно.");
        return;
    }

    // Выделить память
    VkMemoryAllocateInfo MemoryAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = MemoryRequirements.size,
        .memoryTypeIndex = (u32)MemoryType
    };
    VK_CHECK(vkAllocateMemory(config.VkAPI->device.LogicalDevice, &MemoryAllocateInfo, config.VkAPI->allocator, &memory));
    
    if (name) {
        VulkanSetDebugObjectName(config.VkAPI, VK_OBJECT_TYPE_DEVICE_MEMORY, memory, name.c_str());
    }

    // Свяжите память
    VK_CHECK(vkBindImageMemory(config.VkAPI->device.LogicalDevice, handle, memory, 0));  // ЗАДАЧА: настраиваемое смещение памяти.

    // Сообщить о том, что память используется.
    bool IsDeviceMemory = (MemoryFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    MemorySystem::AllocateReport(MemoryRequirements.size, IsDeviceMemory ? Memory::GPULocal : Memory::Vulkan);

    // Создать представление
    if (config.CreateView) {
        // Единое представление, охватывающее все слои.

        ViewCreate(config, -1, view);

        // Создание представлений для каждого слоя.
        if (LayerCount > 1) {
            // Несколько представлений, по одному на слой.
            CreateArray(LayerViews, sizeof(VkImageView) * LayerCount);
            TextureType& ViewType = config.type;
            if (config.type == TextureType::Cube || config.type == TextureType::CubeArray) {
                // ПРИМЕЧАНИЕ: для индивидуальной выборки слоев кубической карты/массива кубической карты тип представления должен быть 2D.
                ViewType = TextureType::_2D;
            }
            for (u32 i = 0; i < LayerCount; ++i) {
                ViewCreate(config, i, LayerViews[i]);
            }
        }
    }
}

// Таблица соответствия типов изображений Vulkan, индексированная по типам текстур Moon.
static VkImageViewType VulkanViewTypes[4] = {
    VK_IMAGE_VIEW_TYPE_2D,
    VK_IMAGE_VIEW_TYPE_2D_ARRAY,
    VK_IMAGE_VIEW_TYPE_CUBE,
    VK_IMAGE_VIEW_TYPE_CUBE_ARRAY};

// Убедитесь, что изменения типов текстур нарушат это, если таблица не будет обновлена.
STATIC_ASSERT(TextureType::Count == (TextureType)(sizeof(VulkanViewTypes) / sizeof(*VulkanViewTypes)), "Количество типов текстур не соответствует количеству в таблице соответствия типов изображений Vulkan.");

void VulkanImage::ViewCreate(Config &config, i32 LayerIndex, VkImageView& view)
{
    VkImageViewCreateInfo ViewCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = handle,
        .viewType = VulkanViewTypes[(u8)config.type],
        .format = format,
    };

    ViewCreateInfo.subresourceRange.aspectMask = config.ViewAspectFlags;
    ViewCreateInfo.subresourceRange.baseMipLevel = 0;
    ViewCreateInfo.subresourceRange.levelCount = config.MipLevels;
    ViewCreateInfo.subresourceRange.baseArrayLayer = LayerIndex < 0 ? 0 : LayerIndex;
    ViewCreateInfo.subresourceRange.layerCount = LayerIndex < 0 ? LayerCount : 1;

    VK_CHECK(vkCreateImageView(config.VkAPI->device.LogicalDevice, &ViewCreateInfo, config.VkAPI->allocator, &view));
    char FormattedName[TEXTURE_NAME_MAX_LENGTH]{};
    MString::Format(FormattedName, "%s_view_idx_%u", name.c_str(), LayerIndex);
    VK_SET_DEBUG_OBJECT_NAME(config.VkAPI, VK_OBJECT_TYPE_IMAGE_VIEW, view, FormattedName);
}

void VulkanImage::TransitionLayout(
    VulkanAPI *VkAPI, 
    VulkanCommandBuffer &CommandBuffer, 
    VkFormat format, 
    VkImageLayout OldLayout, 
    VkImageLayout NewLayout)
{
    VkImageMemoryBarrier barrier {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = OldLayout,
        .newLayout = NewLayout,
        .srcQueueFamilyIndex = VkAPI->device.GraphicsQueueIndex,
        .dstQueueFamilyIndex = VkAPI->device.GraphicsQueueIndex,
        .image = this->handle
    };
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    // Mips
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = MipLevels;
    // Начните с первого слоя.
    barrier.subresourceRange.baseArrayLayer = 0;
    // Переключить все слои одновременно.
    barrier.subresourceRange.layerCount = LayerCount;

    VkPipelineStageFlags SourceStage;
    VkPipelineStageFlags DestStage;

    // ЗАДАЧА: Устанавливайте исходный/целевой этап только один раз... разделите на функции.
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

void VulkanImage::CopyFromBuffer(VulkanAPI* VkAPI, VkBuffer buffer, u64 offset, VulkanCommandBuffer& CommandBuffer)
{
    VkBufferImageCopy region{};
    MemorySystem::ZeroMem(&region, sizeof(VkBufferImageCopy));
    region.bufferOffset = offset;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = LayerCount;

    region.imageExtent.width = this->width;
    region.imageExtent.height = this->height;
    region.imageExtent.depth = 1;

    vkCmdCopyBufferToImage(
        CommandBuffer.handle,
        buffer,
        handle,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);
}

void VulkanImage::CopyToBuffer(VulkanAPI *VkAPI, VkBuffer buffer, VulkanCommandBuffer &CommandBuffer)
{
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = LayerCount;

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

void VulkanImage::CopyPixelToBuffer(VulkanAPI *VkAPI, VkBuffer buffer, u32 x, u32 y, VulkanCommandBuffer &CommandBuffer)
{
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = LayerCount;

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

bool VulkanImage::GenerateMipMaps(VulkanAPI* VkAPI, VulkanCommandBuffer& CommandBuffer)
{
    // Проверьте, поддерживает ли формат изображения линейную передачу.
    VkFormatProperties FormatProperties;
    vkGetPhysicalDeviceFormatProperties(VkAPI->device.PhysicalDevice, format, &FormatProperties);

    if (!(FormatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        MWARN("Формат изображения текстуры не поддерживает линейный перенос! Создание MIP-текстур невозможно.");
        return false;
    }

    VkImageMemoryBarrier barrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = handle
    };
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    // Сгенерировать для всех слоев.
    barrier.subresourceRange.layerCount = LayerCount;
    // Один уровень MIP за раз.
    barrier.subresourceRange.levelCount = 1;

    i32 MipWidth = (i32)width;
    i32 MipHeight = (i32)height;

    // Проведите итерацию каждого подуровня MIP, начиная с 1 (т.е. не с базового уровня/изображения в полном разрешении).
    // Каждый уровень MIP использует предыдущий уровень в качестве исходного материала для операции копирования.
    for (u32 i = 1; i < MipLevels; ++i) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        // Перенесите подресурс MIP-изображения в макет переноса.
        vkCmdPipelineBarrier(
            CommandBuffer.handle,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, 0,
            0, 0,
            1, &barrier
        );

        // Настройте блитинг.
        VkImageBlit blit{};
        // Исходное смещение всегда находится в левом верхнем углу.
        blit.srcOffsets[0] = (VkOffset3D){0, 0, 0};
        // Границы исходного MIP-уровня.
        blit.srcOffsets[1] = (VkOffset3D){MipWidth, MipHeight, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        // Исходный уровень — это предыдущий уровень.
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = LayerCount;
        // Смещение назначения также всегда находится в левом верхнем углу.
        blit.dstOffsets[0] = (VkOffset3D){0, 0, 0};
        // Границы назначения теперь составляют половину ширины/высоты предыдущего уровня, если только предыдущий уровень уже не был равен 1.
        blit.dstOffsets[1] = (VkOffset3D){MipWidth > 1 ? MipWidth / 2 : 1, MipHeight > 1 ? MipHeight / 2 : 1, 1};
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        // Назначение — текущий MIP-уровень.
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = LayerCount;

        // Выполните блитинг для этого слоя.
        vkCmdBlitImage(
            CommandBuffer.handle,
            handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR
        );

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        // Перевести подресурс изображения предыдущего MIP-слоя в макет, читаемый шейдером.
        vkCmdPipelineBarrier(
            CommandBuffer.handle,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, VK_NULL_HANDLE,
            0, VK_NULL_HANDLE,
            1, &barrier
        );

        // Разделить ширину и высоту пополам для следующего уровня, если он есть.
        if (MipWidth > 1) {
            MipWidth /= 2;
        }
        if (MipHeight > 1) {
            MipHeight /= 2;
        }
    }

    // Наконец, перевести последний уровень MIP-карты в макет, читаемый шейдером.
    // Это не было бы обработано в цикле выше, поскольку он всегда переводит предыдущий слой.
    barrier.subresourceRange.baseMipLevel = MipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(
        CommandBuffer.handle,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0, VK_NULL_HANDLE,
        0, VK_NULL_HANDLE,
        1, &barrier
    );

    return true;
}

void VulkanImage::Destroy(VulkanAPI *VkAPI)
{
    if (view) {
        vkDestroyImageView(VkAPI->device.LogicalDevice, view, VkAPI->allocator);
        view = 0;
    }
    if (LayerViews) {
        for (u32 i = 0; i < LayerCount; ++i) {
            vkDestroyImageView(VkAPI->device.LogicalDevice, LayerViews[i], VkAPI->allocator);
        }
        MemorySystem::Free(LayerViews, sizeof(VkImageView) * LayerCount, Memory::Array);
        LayerViews = nullptr;
        LayerCount = 0;
    }
    if (memory) {
        vkFreeMemory(VkAPI->device.LogicalDevice, memory, VkAPI->allocator);
        memory = 0;
    }
    if (handle) {
        vkDestroyImage(VkAPI->device.LogicalDevice, handle, VkAPI->allocator);
        handle = 0;
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
